# WxUI — 코드 리뷰

> 비동기 push 취소, 뷰모델 구독 해제, 월드 타이머 예약 게이트 같은 수명·재진입 처리는 대체로 꼼꼼하다. 이번 변경(호출자 없는 `KillPopup`·`operator==` 제거, 발동 조건 이벤트 구독)도 깨끗하다. 모듈 경계·저작권·인라인 규칙도 지켜진다. 실질 문제는 Modal 스택에서 가려지기만 한 확인 팝업을 종료로 처리하는 1건이다. 비슷한 결의 사소한 누락이 하나 더 있는데, HUD가 가려지거나 걷혀도 진행 중인 메뉴 요청을 취소하지 않는다. 커버리지: 소스 58파일을 모두 읽었다. 서브시스템·푸시 액션·팝업·레이아웃 컴포넌트·HUD·어빌리티 계열 VM은 cpp와 관련 엔진 CommonUI·GAS 코드까지 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 같은 Modal 스택에 위젯이 하나 더 올라오면 살아 있는 확인 팝업이 `Killed`로 끝난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:32-38`, `:115-123`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:85-104`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp:49`
- **범주**: 버그/정확성
- **문제**:
  - 레이어는 `UCommonActivatableWidgetStack`이다(`WxPrimaryGameLayout.cpp:70`). 스택은 새 위젯을 목록에 넣은 뒤 지금 보이던 위젯을 `DeactivateWidget`으로 비활성화한다(엔진 `CommonActivatableWidgetContainer.cpp:209-210`, `:367-375`, `:160-166`). 가려진 위젯은 스택에 남아 있다가 위 위젯이 닫히면 다시 활성화된다(`:296-302`).
  - 그런데 `NativeOnDeactivated`는 모든 비활성화를 종료로 본다. 콜백을 언바인딩한 뒤 `Killed`를 보낸다. 그래서 가려진 팝업은 결과를 이미 `Killed`로 넘긴 채 다시 뜬다. 이때 버튼을 눌러도 `HandleResultChosen`에 콜백이 없어 창만 닫힌다.
  - `ShowConfirmation`에는 진행 중인 요청을 막는 가드가 없다. `UWxHUDLayout`의 `PendingMenuPush` 같은 장치가 없어 지금 콘텐츠로도 재현된다. 팝업 클래스를 스트리밍하는 동안 `WBP_FrontEnd`가 `ShowConfirmationPopup`을 두 번 요청하면, 로드가 끝난 뒤 A·B가 차례로 push된다. A는 B에 가려지는 순간 `Killed`를 받는다. B를 닫고 다시 뜬 A에서 확인을 눌러도 결과가 오지 않는다.
  - 팝업 위에 Modal 위젯을 하나 더 띄우는 흐름(중첩 확인 등)이 생기면 항상 이렇게 된다.
- **제안**: 스택에 가려져서 생긴 비활성화는 종료로 치지 않는다. 방법은 둘 중 하나다.
  - `Killed`를 슬롯이 해제되는 `NativeDestruct`에서만 낸다. 외부 `DeactivateWidget` 경로도 전환 뒤 슬롯이 해제되므로(`:271-292`), 결과가 같은 값으로 조금 늦게 온다.
  - `NativeOnDeactivated`에서 Modal 스택의 `GetWidgetList().Last()`가 자신이 아니면 콜백을 유지한다. 위에 다른 위젯이 이미 등록된 상태라는 뜻이다.
  - 요청 중복까지 막으려면 `ShowConfirmation`에 진행 중 요청 가드를 따로 둔다.
- **확신도**: 높음(메커니즘은 엔진 코드로 확인했다. 현재 재현 경로는 중복 요청뿐이다)

### 2. 🟢 HUD가 가려지거나 걷혀도 진행 중인 메뉴 요청을 취소하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:80-96`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h:41-43`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:98-104`
- **범주**: 버그/정확성
- **문제**:
  - `PendingMenuPush`는 중복 push만 막는다. 푸시 액션은 로드가 끝났을 때 레이아웃이 그대로인지만 확인하고, 요청한 HUD가 아직 활성인지는 보지 않는다(`:98-104`).
  - 그래서 인벤토리·메인 메뉴 클래스를 스트리밍하는 사이에 두 경우가 생기면 메뉴가 뒤늦게 뜬다. 하나는 대화가 시작돼 대화 창이 Game 스택에서 HUD를 가리는 경우이고(엔진 `CommonActivatableWidgetContainer.cpp:160-166`), 다른 하나는 빙의 해제로 `UWxPlayerLayoutComponent::ClearLayout`이 HUD를 걷는 경우다. 메뉴는 대화 창 위나 HUD가 없는 화면에 나타난다.
  - 같은 모양의 요청을 가진 `UWxPlayerLayoutComponent::ClearLayout`(`WxPlayerLayoutComponent.cpp:85-89`)과 `UWxUIManagerSubsystem::CloseDialogueScreen`(`WxUIManagerSubsystem.cpp:352-356`)은 "뒤늦게 나타나지 않도록" 취소한다. HUD만 이 처리가 빠져 있다.
- **제안**: `UWxHUDLayout`에서 `NativeOnDeactivated`를 재정의한다. `PendingMenuPush`가 있으면 `Cancel()`한다(완료 콜백이 멤버를 비운다). 파괴 경로는 `UCommonActivatableWidget::NativeDestruct`가 비활성화를 거치므로 따로 둘 필요가 없다.
- **확신도**: 중간(재현 창은 메뉴 클래스의 비동기 로드가 끝나기 전뿐이다)

### 3. 🟢 동적→네이티브 결과 델리게이트 변환에 람다가 필요하지 않다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`, `:83`
- **범주**: 규칙 위반
- **문제**:
  - `MakeNativeResultDelegate`는 `FWxPopupResultDynamicDelegate`를 복사 캡처해 `CreateWeakLambda`로 감싼다.
  - 네이티브 델리게이트는 `CreateUFunction(UObject*, FName)`으로 같은 UFunction에 약참조로 직접 바인딩할 수 있다(엔진 `Core/Public/Delegates/DelegateSignatureImpl.inl:556`). 인자도 `EWxPopupResult` 하나라 파라미터 레이아웃이 그대로 맞는다.
  - 대체 수단이 있으니 "람다식은 반드시 필요한 경우에만 사용한다"(`CLAUDE.md` 코딩 규칙 3)에 걸린다. 모듈의 유일한 람다다.
- **제안**: 호출부에서 `OnResult.IsBound() ? FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName()) : FWxPopupResultDelegate()`로 만들고, 익명 namespace 헬퍼는 걷는다.
- **확신도**: 중간(동작 동등성은 빌드와 `WBP_FrontEnd` 팝업 결과로 확인해야 한다)

### 4. 🟢 `.cpp` 익명 namespace 헬퍼가 사용자 결정과 어긋난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:19-36`, `:93-102`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp:12-22`, `:35`
- **범주**: 중복/복잡도
- **문제**:
  - 사용자가 정한 방침은 `.cpp`에 익명 namespace도 static 자유 함수도 두지 않는 것이다. 짧은 단일 호출 헬퍼는 호출부에 인라인한다. `WxUILibrary.cpp`의 헬퍼는 3번으로 사라지지만, 이 방침과 어긋나는 헬퍼가 두 파일에 더 남아 있다.
  - `MakeSubtitleContext`는 호출부가 `GetOrCreate` 한 곳뿐이다(`:35`). 조회와 등록도 이미 같은 지역 변수를 쓰므로 헬퍼로 뺄 이유가 없다.
  - 서브시스템의 두 헬퍼는 확인 팝업 종료 처리 수정 때 새로 들어왔다.
    - `HandleConfirmationPushCompleted`는 옆의 `HandleConfirmationPopupReady`처럼 페이로드를 받는 UObject 멤버로 둘 수 있다.
    - `CompleteConfirmationOnce`가 막으려는 이중 전달은 현재 경로에서 생기지 않는다. 완료 콜백은 위젯이 `UWxGamePopup`이 아닐 때만 `Killed`를 낸다. 이는 `SetupPopup`이 돌지 않았거나 push가 활성화 전에 실패한 경우이고, 이때 팝업은 활성화·파괴 이벤트를 받지 않는다.
    - push 뒤의 `bFinished` 분기(`WxAsyncAction_PushWidgetToLayer.cpp:132-143`)는 누군가 `Cancel`을 불러야 돈다. 그런데 `ShowConfirmation`은 액션을 보관하지 않는다.
- **제안**:
  - `MakeSubtitleContext`는 호출부에 인라인한다.
  - `HandleConfirmationPushCompleted`는 private 멤버로 옮긴다.
  - `CompleteConfirmationOnce`는 위 경로 판단을 확인한 뒤 걷고 `ResultCallback`을 그대로 넘긴다. 남겨야 한다면 private static 멤버로 옮긴다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 피드백 `feedback_no_anonymous_namespace`로 정한 방침이다)

### 5. 🟢 이펙트 목록 초기화 여부를 별도 bool로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:92`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:56`, `:61`, `:86`
- **범주**: 중복/복잡도
- **문제**:
  - `bActiveEffectsInitialized`가 뜻하는 "추가·제거 이벤트를 구독했다"는 사실은 이미 ASC 델리게이트에 있다(구독 `:62-63`, 해제 `:71-72`).
  - private `Initialize`는 팩토리(`:26`)에서 새 인스턴스에 한 번만 불린다. `Deinitialize` 뒤에는 `CachedASC`가 비어 `:56` 가드가 먼저 빠지므로, `:86`의 리셋도 효과가 없다.
  - 결국 같은 사실을 두 곳에 두는 분기용 플래그다. 슬롯 VM도 같은 델리게이트를 구독하지만 각자 자기 `this`로 묶여 구분된다.
- **제안**: 가드를 `ASC->OnActiveGameplayEffectAddedDelegateToSelf.IsBoundToObject(this)`로 바꾸고, 플래그 선언과 `Deinitialize`의 리셋을 걷는다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 피드백 `feedback_minimal_design_no_new_layers`로 정한 방침이다)

## 검토 범위
- **깊게 본 파일**:
  - 모듈 소스(각 대응 헤더 포함):
    - `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`
  - 판정 근거로 본 코드:
    - 엔진 CommonUI: `CommonActivatableWidgetContainer.cpp`, `CommonActivatableWidget.cpp`
    - 엔진 GAS: `AbilitySystemComponent_Abilities.cpp`(이벤트 태그 컨테이너 델리게이트), `GameplayEffect.cpp`(제거 통지)
    - 엔진 기타: `PlayerController.cpp`(`SetPause`·`SetPlayer`), `GameModeBase.cpp`(`SetPause`·`ClearPause`), `Player.cpp`·`LocalPlayer.cpp`(`ReceivedPlayerController`), `SWidgetSwitcher.cpp`(`RemoveSlot`)
    - 발행측: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:74-82`(발동 조건 이벤트)
- **훑은 파일**:
  - 나머지 소스 전부:
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `WxViewModel_Character.cpp`, `WxViewModel_Subtitle.cpp`, `WxViewModel_Item.cpp`, `WxViewModel_Interaction.cpp`, `WxViewModel_Indicator.cpp`, `WxViewModelResolver_PlayerCharacter.cpp`, `WxMVVMConversionLibrary.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `WxActivatableWidget.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`
    - `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`
    - `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`
  - 판정 근거 확인용: `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩·이벤트 그래프)는 보지 않았다. 에셋 사용 여부는 `.uasset` 바이너리 문자열 검색으로만 확인했다.
    - `ShowConfirmationPopup` 호출은 `WBP_FrontEnd` 1건이다.
    - `UI.Layer.Modal`을 직접 참조하는 에셋도 이 1건뿐이다.
    - `OnHandleBackAction` 오버라이드, 제거된 `KillPopup` 참조, `SetGamePaused` 호출은 0건이다.
  - 네트워크 클라이언트에서 `SetPlayer`(레이아웃 생성)와 빙의 복제(HUD push)의 도착 순서는 확인하지 못했다. 싱글·리슨 호스트 기준으로만 판단했다.
  - 뺀 항목:
    - 가치가 낮아 뺀 것: `AWxIndicator::BeginPlay`가 베이스(`AActor`, protected)보다 넓은 public 절에 선언돼 있다(`Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h:31`). 외부 호출자가 없고 접근만 넓힌다.
    - 의도된 결정이라 뺀 것:
      - 확인 전용 팝업에서 뒤로 가기는 입력만 소비한다(`.codex/worklog/2026-09-15-확인-팝업-종료-결과.md`).
      - 월드 타이머 예약은 일시정지 중에 돌지 않고, 게이지 갱신은 `UWxViewModel_Effect`의 코어 티커로 유지한다(`.claude/worklog/2026-09-14-코드리뷰-반영-월드타이머-전환.md`).
      - `UWxActivatableWidget::GetDesiredInputConfig`는 `All`을 처리하지 않는다.
    - 도달 경로가 없어 뺀 것:
      - 다른 정지 주체가 먼저 정지를 걸면 `PC->SetPause`가 우리 `CanUnpause` 대리자를 등록하지 않는다. 코드·에셋에 다른 정지 주체가 없다.
      - 사망 화면에만 대화 창 같은 취소·회수 경로가 없는 구조 비대칭은 그대로다. 지난 리뷰의 판단을 뒤집을 새 도달 경로는 찾지 못했다.
      - `UWxConfirmationPopup::NativeOnHandleBackAction`이 `Super`를 부르지 않는다. BP 오버라이드가 없다.
  - 모듈 규칙은 전 파일을 확인했다.
    - `WxUI.Build.cs`·`WxUI.uplugin`은 Wx 플러그인 중 `WxCore`만 참조한다. Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다.
    - 소스 58파일과 `WxUI.Build.cs`의 첫 줄 저작권이 모두 맞다.
    - 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고, 둘 다 사유 주석이 있다.
  - 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).

---
*문서 기준 커밋 `7d1d0374` · 리뷰일 2026-09-15 · 소스 58파일 — `/module-review`로 갱신*
