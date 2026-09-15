# WxUI — 코드 리뷰

> 지난 리뷰의 핵심 문제는 해소됐다. 확인 팝업이 스택에 가려지면 `Killed`로 끝나던 문제는 결과를 사용자 선택에서만 내는 Lyra식 계약과 동기 생성으로 정리됐다. 이펙트 초기화 플래그와 서브시스템의 익명 namespace 헬퍼도 걷혔다. 이번 변경 9파일에서 새 결함은 없고, 남은 발견은 모두 사소하다. 동기화로 호출자를 잃은 푸시 액션 콜백 1건과 지난 리뷰에서 넘어온 3건이다. 커버리지: 소스 58파일을 모두 읽었다. 변경 파일과 그 호출 경로(푸시 액션·레이아웃·HUD·레이아웃 컴포넌트·라이브러리)는 엔진 CommonUI 컨테이너 코드까지 대조해 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟢 HUD가 가려지거나 걷혀도 진행 중인 메뉴 요청을 취소하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:80-96`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h:41-43`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:98-104`
- **범주**: 버그/정확성
- **문제**:
  - `PendingMenuPush`는 중복 push만 막는다. 푸시 액션은 로드가 끝났을 때 레이아웃이 그대로인지만 보고, 요청한 HUD가 아직 활성인지는 보지 않는다(`:98-104`).
  - 인벤토리·메인 메뉴 클래스를 스트리밍하는 사이에 다음 두 일이 생기면 메뉴가 뒤늦게 뜬다.
    - 대화가 시작돼 대화 창이 Game 스택에서 HUD를 가린다(`WxUIManagerSubsystem.cpp:310-314`, 엔진 `CommonActivatableWidgetContainer.cpp:160-166`). 메뉴가 대화 창 위에 나타난다.
    - 빙의 해제로 `UWxPlayerLayoutComponent::ClearLayout`이 HUD를 걷는다(`WxPlayerLayoutComponent.cpp:92-98`). 메뉴가 HUD 없는 화면에 나타난다.
  - 같은 모양의 요청을 가진 `UWxPlayerLayoutComponent::ClearLayout`(`WxPlayerLayoutComponent.cpp:85-89`)과 `UWxUIManagerSubsystem::CloseDialogueScreen`(`WxUIManagerSubsystem.cpp:337-341`)은 "뒤늦게 나타나지 않도록" 취소한다. HUD에만 이 처리가 없다.
- **제안**: `UWxHUDLayout`에서 `NativeOnDeactivated`를 재정의해 `PendingMenuPush`가 있으면 `Cancel()`한다(완료 콜백이 멤버를 비운다). 파괴 경로는 `UCommonActivatableWidget::NativeDestruct`가 비활성화를 거치므로 따로 둘 필요가 없다.
- **확신도**: 중간(재현 창은 메뉴 클래스가 처음 스트리밍되는 동안뿐이다. 이미 로드된 클래스는 `Activate` 안에서 곧바로 push된다)

### 2. 🟢 `SetBeforePushCallback`이 호출자를 잃은 데드 코드가 됐다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h:34-35`, `:61`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:62-65`, `:120`, `:161-162`
- **범주**: 중복/복잡도
- **문제**:
  - 유일한 호출자였던 `UWxUIManagerSubsystem::ShowConfirmation`이 `2d0871b41`에서 동기 생성(`CreateWidget` → `SetupPopup` → push)으로 바뀌었다. 이제 저장소 C++에 `SetBeforePushCallback` 호출이 없다. `UFUNCTION`이 아니라 BP 경로도 없다.
  - 선언·멤버·실행 줄(`:120`)·언바인딩(`:162`)이 아무 일도 하지 않는다.
  - `:161` 주석("콜백 페이로드가 붙잡고 있는 참조를 여기서 놓는다")은 사라진 `TStrongObjectPtr` 서술자 페이로드를 설명하던 것이다. 남은 완료 콜백 3곳(`WxHUDLayout.cpp:88`, `WxPlayerLayoutComponent.cpp:72`, `WxUIManagerSubsystem.cpp:312`)은 모두 페이로드 없는 `CreateUObject`다.
- **제안**: `SetBeforePushCallback` 선언·정의, `BeforePushCallback` 멤버, `:120` 실행 줄, `:161-162` 주석과 언바인딩을 걷는다. BP 비동기 노드용 `BeforePush` 동적 델리게이트는 그대로 둔다.
- **확신도**: 높음

### 3. 🟢 동적→네이티브 결과 델리게이트 변환에 람다가 필요하지 않다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`, `:83`
- **범주**: 규칙 위반
- **문제**:
  - `MakeNativeResultDelegate`는 `FWxPopupResultDynamicDelegate`를 복사 캡처해 `CreateWeakLambda`로 감싼다.
  - 네이티브 델리게이트는 `CreateUFunction(UObject*, FName)`으로 같은 UFunction에 약참조로 직접 바인딩할 수 있다(엔진 `Core/Public/Delegates/DelegateSignatureImpl.inl:556`). 인자도 `EWxPopupResult` 하나라 파라미터 레이아웃이 그대로 맞는다.
  - 대체 수단이 있으니 "람다식은 반드시 필요한 경우에만 사용한다"(`CLAUDE.md` 코딩 규칙 3)에 걸린다. 모듈 소스의 유일한 람다다.
- **제안**: 호출부에서 `OnResult.IsBound() ? FWxPopupResultDelegate::CreateUFunction(OnResult.GetUObject(), OnResult.GetFunctionName()) : FWxPopupResultDelegate()`로 만들고 익명 namespace 헬퍼를 걷는다.
- **확신도**: 중간(동작 동등성은 빌드와 `WBP_FrontEnd` 팝업 결과로 확인해야 한다)

### 4. 🟢 단일 호출 헬퍼 `MakeSubtitleContext`가 익명 namespace에 남아 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp:12-22`, `:35`
- **범주**: 중복/복잡도
- **문제**:
  - 호출부는 `UWxViewModel_Subtitle::GetOrCreate` 한 곳뿐이다(`:35`). 헬퍼 주석이 말하는 "등록·조회가 같은 값"은 이미 같은 지역 변수 `Context`로 보장된다(`:36`, `:43`).
  - 사용자는 파일 로컬 namespace 헬퍼를 풀어 쓰는 쪽을 택해 왔다(메모리 `prefer-explicit-over-tiny-helpers`). 서브시스템의 같은 모양 헬퍼 2개는 이번 변경에서 걷혔고, 3번을 고치면 `WxUILibrary.cpp`의 것도 사라진다. 그러면 모듈에 이것 하나만 남는다.
- **제안**: 문맥 구성 3줄을 `GetOrCreate` 안으로 옮기고 namespace 블록을 걷는다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 선호로 정한 방침이다)

## 검토 범위
- **깊게 본 파일**:
  - 이번 변경분(각 대응 헤더 포함):
    - `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`
    - `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`
  - 변경분의 호출 경로:
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`
  - 판정 근거로 본 코드:
    - 엔진 CommonUI `CommonActivatableWidgetContainer.cpp`: `AddWidgetInstance`, `SetSwitcherIndex`, `HandleActiveIndexChanged`, `ReleaseWidget`
    - 엔진 CommonUI `SCommonAnimatedSwitcher.cpp`: 전환 시간 0이면 `TransitionToIndex`가 동기로 활성화한다. 그래서 팝업 위에 팝업을 겹쳐도 정지가 풀린 틱이 끼지 않는다.
    - 엔진 CommonUI `CommonActivatableWidget.cpp`: `NativeDestruct`, `NativeOnHandleBackAction`
    - 엔진 Core `DelegateSignatureImpl.inl`: `CreateUFunction`
    - 작업 기록: `.claude/worklog/2026-09-15-가려진-확인-팝업-종료-판정.md`, `.codex/worklog/2026-09-15-확인-팝업-동기-로드.md`
- **훑은 파일**:
  - 나머지 소스 전부:
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `WxViewModel_Ability.cpp`, `WxViewModel_Effect.cpp`, `WxViewModel_Attribute.cpp`, `WxViewModel_Character.cpp`, `WxViewModel_Subtitle.cpp`, `WxViewModel_Item.cpp`, `WxViewModel_Interaction.cpp`, `WxViewModel_Indicator.cpp`, `WxViewModelResolver_PlayerCharacter.cpp`, `WxMVVMConversionLibrary.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `WxActivatableWidget.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`
    - `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`
    - `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩·이벤트 그래프)는 보지 않았다. 에셋 사용 여부는 `.uasset` 바이너리 문자열 검색으로만 확인했다.
    - `ShowConfirmationPopup` 호출은 `WBP_FrontEnd` 1건이다.
    - `DeactivateOwningActivatable` 호출은 `WBP_Close` 1건이다. `WBP_Close`는 `WBP_Inventory`·`WBP_MainMenu`에만 들어 있어, 확인 팝업을 밖에서 닫는 경로는 에셋에도 없다.
  - 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).
  - 해소를 확인해 뺀 지난 발견:
    - 가려진 확인 팝업의 `Killed`: `UWxConfirmationPopup`의 `NativeOnDeactivated`·`NativeDestruct` 재정의가 제거됐다.
    - `bActiveEffectsInitialized` 플래그: `IsBoundToObject(this)` 판정으로 바뀌었다.
    - 서브시스템 익명 namespace 헬퍼 2개(`CompleteConfirmationOnce`·`HandleConfirmationPushCompleted`): 제거됐다.
    - `AWxIndicator::BeginPlay` 접근 지정자: protected로 정정됐다.
  - 의도된 결정이라 뺀 것:
    - 표시된 팝업을 밖에서 닫으면(레이아웃 교체·`RemoveWidget`·외부 비활성화) 결과가 오지 않는다. `EWxPopupResult::Killed` 주석(`WxGamePopup.h:14-17`)과 작업 기록에 명시된 Lyra식 계약이다.
    - 확인 팝업 중복 요청 가드는 두지 않는다(작업 기록에서 UX 정책으로 보류). 동기 push라 겹친 팝업은 가려진 동안에도 콜백을 유지해 각자 결과를 낸다.
    - `UWxConfirmationPopup::NativeOnHandleBackAction`은 `Super`를 부르지 않고, 확인 전용 팝업에선 입력만 소비한다. 엔진 기본값인 무조건 비활성화를 대체하려는 것이고 BP 오버라이드도 없다.
    - `UWxActivatableWidget::GetDesiredInputConfig`는 `All`을 처리하지 않는다.
    - 뷰모델의 월드 타이머 예약은 일시정지 중에 돌지 않는다(`.claude/worklog/2026-09-14-코드리뷰-반영-월드타이머-전환.md`).
  - 도달 경로·가치가 없어 뺀 것:
    - `UWxUIManagerSubsystem::Initialize`의 팝업 클래스 동기 로드는 서브시스템에 `ShouldCreateSubsystem` 재정의가 없어 데디케이티드 서버에서도 돈다. 서버 타깃이 없고(`Source/Wx.Target.cs`·`WxEditor.Target.cs`만 있음) 멀티 정책도 정해지지 않았다.
    - `HandlePlayerControllerSet`은 추적 PC를 먼저 비운다. 그래서 정지 위젯이 레이아웃과 함께 걷힐 때 `RefreshGamePause`가 조기 반환해 옛 PC의 정지를 풀지 않는다. 같은 월드에서 PC를 바꾸는 코드(`SwitchController`·`SwapPlayerControllers` 등)가 없다.
    - 다른 정지 주체가 먼저 정지를 걸면 `PC->SetPause`가 우리 `CanUnpause` 대리자를 등록하지 않는다. 코드·에셋에 다른 정지 주체가 없다.
    - 사망 화면에만 대화 창 같은 취소·회수 경로가 없는 구조 비대칭은 그대로다. 지난 판단을 뒤집을 새 도달 경로는 찾지 못했다.
  - 모듈 규칙은 전 파일을 확인했다.
    - `WxUI.Build.cs`·`WxUI.uplugin`은 Wx 플러그인 중 `WxCore`만 참조한다. Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다.
    - 소스 58파일과 `WxUI.Build.cs`의 첫 줄 저작권이 모두 맞다.
    - 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고, 둘 다 사유 주석이 있다.

---
*문서 기준 커밋 `e0106372a` · 리뷰일 2026-09-16 · 소스 58파일 — `/module-review`로 갱신*
