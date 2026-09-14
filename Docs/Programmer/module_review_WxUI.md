# WxUI — 코드 리뷰

> 지난 리뷰 뒤 들어간 확인 팝업 종료 처리(요청당 1회만 전달되는 공유 델리게이트, 버튼·뒤로 가기·외부 비활성화·push 실패·레이아웃 제거)와 쿨다운 월드 타이머 전환(핸들을 먼저 놓고 다음 틱 재예약)은 대부분의 경로에서 정확하다. 모듈 경계·저작권·인라인 규칙도 깨끗하다. 남은 실질 문제는 Modal 스택에서 가려지기만 한 팝업을 종료로 처리하는 1건이고, 나머지는 사소하다. 커버리지: 소스 58파일을 모두 읽었고, 변경 6파일·서브시스템·푸시 액션·팝업·MVVM 핵심 VM·인디케이터·레이아웃 컴포넌트는 cpp와 관련 엔진 CommonUI 코드까지 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 같은 Modal 스택에 다른 위젯이 올라오면 살아 있는 확인 팝업이 `Killed`로 끝난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:32-38`, `:121-129`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:85-104`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp:70`
- **범주**: 버그/정확성
- **문제**: 레이어는 `UCommonActivatableWidgetStack`이다. 스택은 새 위젯을 올릴 때 지금 보이던 위젯을 `DeactivateWidget`으로 비활성화한다(엔진 `CommonActivatableWidgetContainer.cpp:162-166`, `:367-375`). 이 위젯은 스택에 남아 있다가 위 위젯이 닫히면 다시 활성화된다(`:296-302`). 그런데 새 `NativeOnDeactivated`는 비활성화를 곧 종료로 보고, 콜백을 언바인딩한 뒤 `Killed`를 보낸다. 가려진 팝업은 결과를 이미 `Killed`로 넘긴 채 나중에 다시 뜨고, 이때 버튼을 눌러도 `HandleResultChosen`에 콜백이 없어 창만 닫힌다. `ShowConfirmation`에는 진행 중인 요청을 막는 가드도 없다(`UWxHUDLayout`의 `PendingMenuPush` 같은 것이 없다). 그래서 지금 콘텐츠로도 재현된다. 팝업 클래스를 처음 스트리밍하는 동안 `WBP_FrontEnd`에서 `ShowConfirmationPopup`을 두 번 요청하면, 로드 완료 뒤 A·B가 차례로 push되고 A는 B에 가려지는 순간 `Killed`를 받는다. B를 닫고 다시 뜬 A에서 확인을 눌러도 결과가 오지 않는다. 팝업 위에 Modal 위젯을 하나 더 띄우는 흐름(중첩 확인 등)이 생기면 항상 이렇게 된다.
- **제안**: 스택에 가려져서 생긴 비활성화는 종료로 치지 않는다. 방법은 둘 중 하나다. 하나는 `Killed`를 스택이 슬롯을 해제해 Slate 위젯이 파괴되는 `NativeDestruct`에서만 내는 것이다. 외부 `DeactivateWidget` 경로도 전환 뒤 해제되므로 한 프레임 늦게 같은 결과가 온다. 다른 하나는 `NativeOnDeactivated`에서 소속 스택의 `GetWidgetList().Last()`가 자신이 아니면(위에 다른 위젯이 등록된 상태) 콜백을 유지하는 것이다. 요청 중복은 필요하면 `ShowConfirmation`에 진행 중 요청 가드를 따로 둔다.
- **확신도**: 높음(메커니즘은 엔진 코드로 확인했다. 현재 재현 경로는 중복 요청뿐이다)

### 2. 🟢 동적→네이티브 결과 델리게이트 변환에 람다가 필요하지 않다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`, `:83`
- **범주**: 규칙 위반
- **문제**: `MakeNativeResultDelegate`는 `FWxPopupResultDynamicDelegate`를 복사해 `CreateWeakLambda`로 감싼다. 네이티브 델리게이트는 `CreateUFunction(UObject*, FName)`으로 같은 UFunction에 약참조로 직접 바인딩할 수 있고(엔진 `DelegateSignatureImpl.inl:556`), 결과 인자도 `EWxPopupResult` 하나라 파라미터 레이아웃이 그대로 맞는다. 대체 수단이 있으니 "람다식은 반드시 필요한 경우에만 사용한다"(`CLAUDE.md` 코딩 규칙 3)에 걸린다. 모듈의 유일한 람다다.
- **제안**: 호출부에서 `OnResult.IsBound() ? FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName()) : FWxPopupResultDelegate()`로 바꾸고, 익명 namespace 헬퍼는 걷는다.
- **확신도**: 중간(동작 동등성은 빌드와 `WBP_FrontEnd` 팝업 결과로 확인해야 한다)

### 3. 🟢 호출자 없는 `KillPopup`과 `FWxConfirmationPopupAction::operator==`
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:35`, `:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:5-8`, `:91-93`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxConfirmationPopup.h:22`, `:52`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:115-119`
- **범주**: 중복/복잡도
- **문제**: `KillPopup`은 UFUNCTION이 아닌데 저장소 C++ 어디에서도 부르지 않는다. 이번 변경 뒤로는 `DeactivateWidget()`만 불러도 `NativeOnDeactivated`가 `Killed`를 내므로 기능까지 겹친다. `operator==`도 비교하는 곳이 없고 `TStructOpsTypeTraits` 특수화도 없어 리플렉션에서도 쓰이지 않는다.
- **제안**: 베이스·파생의 `KillPopup` 선언·정의와 `operator==`를 지운다. `HandleResultChosen` 주석의 "강제 종료" 언급(`WxConfirmationPopup.h:52`)도 함께 정리한다.
- **확신도**: 높음

### 4. 🟢 `AWxIndicator::BeginPlay`가 베이스보다 넓은 public으로 재정의돼 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h:31`
- **범주**: 설계/구조
- **문제**: 직접 베이스 `AActor::BeginPlay`는 protected인데(엔진 `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:2119`, `:2125`) 여기서는 public 절에 선언해 외부에서 직접 부를 수 있게 열어 둔다. override는 베이스 접근 지정자를 그대로 둔다는 프로젝트 관례와 어긋나고, 모듈의 다른 override는 모두 베이스와 일치한다.
- **제안**: `BeginPlay` 선언을 protected 절로 옮기고 cpp 정의 순서도 헤더에 맞춘다.
- **확신도**: 높음

### 5. 🟢 이펙트 목록 초기화 여부를 별도 bool로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:92`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:56`, `:62`, `:87`
- **범주**: 중복/복잡도
- **문제**: `bActiveEffectsInitialized`가 뜻하는 "추가·제거 이벤트를 구독했다"는 사실은 이미 ASC 델리게이트에 남아 있다(구독 `:63-64`, 해제 `:72-73`). `Initialize`는 팩토리에서 한 번만 불리고, 해제 뒤에는 `CachedASC`가 비어 가드가 먼저 빠진다. 그러니 같은 사실을 두 곳에 두는 분기용 멤버 플래그일 뿐이다. 기존 상태에서 파생한다는 프로젝트 방침과 어긋난다.
- **제안**: 가드를 `ASC->OnActiveGameplayEffectAddedDelegateToSelf.IsBoundToObject(this)`로 바꾸고 플래그 선언과 `Deinitialize`의 리셋을 걷는다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 피드백으로 정해진 방침이다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp` (각 대응 헤더 포함), 엔진 `CommonActivatableWidget.cpp`·`CommonActivatableWidgetContainer.cpp`·`PlayerController.cpp`(`SetPause`)·`GameModeBase.cpp`(`ClearPause`)
- **훑은 파일**: 나머지 소스 전부 — `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `WxViewModel_Character.cpp`, `WxViewModel_Subtitle.cpp`, `WxViewModel_Item.cpp`, `WxViewModel_Interaction.cpp`, `WxViewModel_Indicator.cpp`, `WxViewModelResolver_PlayerCharacter.cpp`, `WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `WxButtonBase.cpp`, `WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, 판정 근거 확인용 `Source/WxGame/Framework/WxRespawnLibrary.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`·`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩·이벤트 그래프)는 보지 않았다. 에셋 사용 여부는 `.uasset` 바이너리 문자열 검색으로만 확인했다(`ShowConfirmationPopup` 호출은 `WBP_FrontEnd` 1건, `UI.Layer.Modal` 직접 참조 에셋은 0건). 팝업 WBP가 화면 전체를 덮어 아래 버튼 재클릭을 막는지는 알 수 없어, 1번의 동기 로드 상태 재현성은 확인하지 못했다.
  - 게임→프론트엔드 `OpenLevel` 이동 때 GameInstance 서브시스템이 쥔 `PrimaryGameLayout`과 MVVM 소스가 옛 월드를 GC에서 붙잡는지는 코드만으로 결론 내지 못했다(런타임 월드 누수 경고 확인 필요).
  - 지난 리뷰 2번(사망 화면에만 대화 창 같은 취소·재확인·회수 경로가 없음)은 뺐다. 구조 비대칭은 그대로지만 도달 경로를 찾지 못했다. 사망한 플레이어 폰을 `UWxRespawnLibrary::RequestRespawn` 밖에서 빙의 해제하거나 파괴하는 코드가 없다(`UWxAbility_Death::PendingDestroyTime` 기본 0, 래그돌 진입 시 이동 비활성). `RequestRespawn`이 실패하면 죽은 폰을 다시 빙의해 화면이 유지된다.
  - 의도된 결정이라 뺀 것:
    - 확인 전용 팝업에서 뒤로 가기가 입력만 소비하고 닫지 않는 점(`.codex/worklog/2026-09-15-확인-팝업-종료-결과.md`).
    - 월드 타이머 예약이 일시정지 중 돌지 않는 점(`2026-09-14-코드리뷰-반영-월드타이머-전환`).
    - `UWxActivatableWidget::GetDesiredInputConfig`의 `All` 처리.
  - `CLAUDE.md`에 근거 규칙이 없어 뺀 것:
    - cpp 정의 순서가 헤더 선언 순서와 다른 파일(`WxConfirmationPopup.cpp`, `WxViewModel_Ability.cpp`).
    - 단일 호출부 헬퍼 `MakeSubtitleContext`.
    - `UWxConfirmationPopup::NativeOnHandleBackAction`이 `Super`를 부르지 않아 BP `OnHandleBackAction` 오버라이드를 무시하는 점. 오버라이드 에셋 여부는 확인하지 못했다.
  - 모듈 규칙은 전 파일 확인했다.
    - `WxUI.Build.cs`·`WxUI.uplugin`은 `WxCore` 외 Wx 플러그인을 참조하지 않고, Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다.
    - 소스 58파일 모두 첫 줄 저작권이 맞다.
    - 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고 사유 주석이 있다.
  - 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).

---
*문서 기준 커밋 `993d2a031` · 리뷰일 2026-09-15 · 소스 58파일 — `/module-review`로 갱신*
