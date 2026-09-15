# WxUI — 코드 리뷰

> 모듈은 건강하다. 레이어 push·정지 재평가·뷰모델 공유와 수명 규약이 일관되게 지켜지고, 심각하거나 개선이 필요한 결함은 찾지 못했다. 남은 발견은 모두 사소하다. 지난 리뷰 이후 코드 변경은 `WxViewModel_Ability` 이벤트 태그 개명 2파일뿐이다. 지난 발견 4건은 현재 코드로 다시 확인해 그대로 남겼고, 폰 교체 경로에서 도달하지 않는 게이트 1건을 새로 찾았다. 커버리지: 소스 58파일을 모두 읽었다. 제어 흐름의 중심(서브시스템·push 액션·HUD·레이아웃 컴포넌트·팝업)과 뷰모델 3종은 엔진 코드와 대조해 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟢 HUD가 가려지거나 걷혀도 진행 중인 메뉴 요청을 취소하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:80-96`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h:41-43`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:98-104`
- **범주**: 버그/정확성
- **문제**:
  - `PendingMenuPush`는 중복 push만 막는다. 푸시 액션은 로드가 끝났을 때 레이아웃이 그대로인지만 본다(`:98-104`). 요청한 HUD가 아직 활성인지는 보지 않는다.
  - 인벤토리·메인 메뉴 클래스를 스트리밍하는 사이에 다음 일이 생기면 메뉴가 뒤늦게 뜬다.
    - 대화가 시작돼 대화 창이 Game 스택에서 HUD를 가린다(`WxUIManagerSubsystem.cpp:310-314`, 엔진 `CommonActivatableWidgetContainer.cpp:158-166`). 그러면 메뉴가 대화 창 위에 나타난다.
    - 빙의 해제로 `UWxPlayerLayoutComponent::ClearLayout`이 HUD를 걷는다(`WxPlayerLayoutComponent.cpp:92-99`). 그러면 메뉴가 HUD 없는 화면에 나타난다.
  - 같은 모양의 요청을 가진 `UWxPlayerLayoutComponent::ClearLayout`(`WxPlayerLayoutComponent.cpp:85-89`)과 `UWxUIManagerSubsystem::CloseDialogueScreen`(`WxUIManagerSubsystem.cpp:337-341`)은 "뒤늦게 나타나지 않도록" 요청을 취소한다. HUD에만 이 처리가 없다.
- **제안**: `UWxHUDLayout`에서 `NativeOnDeactivated`를 재정의해 `PendingMenuPush`가 있으면 `Cancel()`한다. 완료 콜백이 멤버를 비운다. 파괴 경로는 따로 둘 필요가 없다. `UCommonActivatableWidget::NativeDestruct`가 비활성화를 거친다(엔진 `CommonActivatableWidget.cpp:61-68`).
- **확신도**: 중간(재현 창은 메뉴 클래스를 처음 스트리밍하는 동안뿐이다. 이미 로드된 클래스는 `Activate` 안에서 곧바로 push된다)

### 2. 🟢 레이아웃 컴포넌트의 "같은 폰" 게이트에 도달하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp:54-69`, `:44-48`
- **범주**: 중복/복잡도
- **문제**:
  - `:44-48`은 옛 폰과 새 폰이 다르면 먼저 `ClearLayout`을 부른다. 이 호출이 `LayoutWidget`과 `PendingLayoutPush`를 비운다(`:85-89`, `:100`). 그래서 `:60-69`의 두 게이트는 두 폰이 같은 알림에서만 반환할 수 있다.
  - 그런 알림은 오지 않는다.
    - 엔진은 폰이 실제로 바뀔 때만 `OnPossessedPawnChanged`를 방송한다. `Possess`(`Controller.cpp:346-350`), `UnPossess`(`:400-404`), `OnRep_Pawn`(`:570-572`)이 모두 그렇다.
    - `Possess`의 알림 강제 분기도 옛 폰을 nullptr로 보낸다.
    - BeginPlay 따라잡기(`:27`)도 옛 폰을 nullptr로 넘긴다.
  - 결국 두 게이트는 `Contains(nullptr)`와 이미 null인 멤버만 검사한다. `:60` 주석("같은 Pawn 알림은 기존 HUD를 유지한다")은 일어나지 않는 경우를 설명한다.
  - 게이트는 폰을 갈아타도 HUD를 유지하던 설계(`86b134d67`, 08-28)에서 왔다. 폰 교체 때 HUD를 걷는 분기가 앞에 붙으면서(`9a03745db`, 09-06) 역할을 잃었다.
- **제안**: `:60-69`를 걷는다. 그러면 `:54-58`의 레이아웃 조회도 게이트 말고는 쓰는 곳이 없어 함께 걷을 수 있다. 레이아웃이 없을 때는 푸시 액션이 실패 완료로 끝낸다(`WxAsyncAction_PushWidgetToLayer.cpp:30-37`).
- **확신도**: 높음

### 3. 🟢 `SetBeforePushCallback`이 호출자 없는 데드 코드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h:34-35`, `:61`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:62-65`, `:120`, `:161-162`
- **범주**: 중복/복잡도
- **문제**:
  - 저장소 C++(`Source/`·`Plugins/`)에 `SetBeforePushCallback` 호출이 없다. `UFUNCTION`이 아니라 BP 경로도 없다. 선언·멤버·실행 줄(`:120`)·언바인딩(`:162`)이 아무 일도 하지 않는다.
  - `:161` 주석("콜백 페이로드가 붙잡고 있는 참조를 여기서 놓는다")은 이미 사라진 서술자 페이로드를 설명한다. 남은 완료 콜백 3곳은 모두 페이로드 없는 `CreateUObject`다(`WxHUDLayout.cpp:88-89`, `WxPlayerLayoutComponent.cpp:72-73`, `WxUIManagerSubsystem.cpp:312-313`).
- **제안**: `SetBeforePushCallback` 선언·정의, `BeforePushCallback` 멤버, `:120` 실행 줄, `:161-162` 주석과 언바인딩을 걷는다. BP 비동기 노드용 `BeforePush` 동적 델리게이트는 그대로 둔다.
- **확신도**: 높음

### 4. 🟢 동적→네이티브 결과 델리게이트 변환에 람다가 필요하지 않다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`, `:83`
- **범주**: 규칙 위반
- **문제**:
  - `MakeNativeResultDelegate`는 `FWxPopupResultDynamicDelegate`를 복사 캡처해 `CreateWeakLambda`로 감싼다. 모듈 소스의 유일한 람다다.
  - 네이티브 델리게이트는 `CreateUFunction(UObject*, FName)`으로 같은 UFunction에 약참조로 직접 바인딩할 수 있다(엔진 `DelegateSignatureImpl.inl:555-561`). 인자도 `EWxPopupResult` 하나라 파라미터 레이아웃이 그대로 맞는다.
  - 대체 수단이 있으니 "람다식은 반드시 필요한 경우에만 사용한다"(`CLAUDE.md` 코딩 규칙 3)에 걸린다.
- **제안**: 익명 namespace 헬퍼를 걷고 호출부에서 직접 만든다.
  - const 참조의 `GetUObject()`는 `const UObject*`를 돌려준다(엔진 `ScriptDelegates.h:908`). `CreateUFunction`은 비const 객체로 `ProcessEvent`를 불러야 하므로 먼저 지역 복사본을 만든다.
  - 복사본이 바인딩돼 있으면 `FWxPopupResultDelegate::CreateUFunction(Target.GetUObject(), Target.GetFunctionName())`, 아니면 빈 델리게이트를 넘긴다.
- **확신도**: 중간(동작 동등성은 빌드와 `WBP_FrontEnd` 팝업 결과로 확인해야 한다)

### 5. 🟢 단일 호출 헬퍼 `MakeSubtitleContext`가 익명 namespace에 남아 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp:12-22`, `:35`
- **범주**: 중복/복잡도
- **문제**:
  - 호출부는 `UWxViewModel_Subtitle::GetOrCreate` 한 곳뿐이다(`:35`). 헬퍼 주석이 말하는 "등록·조회가 같은 값"은 이미 같은 지역 변수 `Context`로 보장된다(`:36`, `:43`).
  - 사용자는 파일 로컬 namespace 헬퍼를 풀어 쓰는 쪽을 택해 왔다(메모리 `prefer-explicit-over-tiny-helpers`). 4번을 고치면 모듈의 익명 namespace 헬퍼는 이것 하나만 남는다.
- **제안**: 문맥 구성 3줄을 `GetOrCreate` 안으로 옮기고 namespace 블록을 걷는다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 선호로 정한 방침이다)

## 검토 범위
- **깊게 본 파일**:
  - 제어 흐름(각 대응 헤더 포함):
    - `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`
  - 뷰모델·인디케이터:
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`(이번 변경분 포함)
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`
  - 판정 근거로 본 엔진 코드(UE 5.8):
    - `Engine/Private/Controller.cpp`: `Possess`·`UnPossess`·`OnRep_Pawn`의 빙의 알림 조건
    - CommonUI `CommonActivatableWidgetContainer.cpp`: `RemoveWidget`, `SetSwitcherIndex`, `HandleActiveWidgetDeactivated`, `HandleActiveIndexChanged`, 스택의 0번 빈 슬롯
    - CommonUI `CommonActivatableWidget.cpp`: `NativeDestruct`
    - `Engine/Private/GameModeBase.cpp`: `ClearPause`의 `FCanUnpause` 처리
    - Core `DelegateSignatureImpl.inl`·`ScriptDelegates.h`: `CreateUFunction`, `GetUObject`
  - 작업 기록: `.claude/worklog/2026-08-27-HUD-재push-게이트.md`, `.claude/worklog/2026-09-14-코드리뷰-반영-월드타이머-전환.md`, `.claude/worklog/2026-09-01-뷰모델-teardown-규약.md`, `.codex/worklog/2026-09-15-Ability-쿨다운-월드타이머.md`, `.codex/worklog/2026-09-15-ActionPhase-이벤트-UI-갱신.md`
- **훑은 파일**:
  - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `WxViewModel_Character.cpp`, `WxViewModel_Item.cpp`, `WxViewModel_Subtitle.cpp`, `WxViewModel_Interaction.cpp`, `WxViewModel_Indicator.cpp`, `WxViewModelResolver_PlayerCharacter.cpp`, `WxMVVMConversionLibrary.cpp`
  - `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`
  - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`
  - `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `WxButtonBase.cpp`
  - `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`
  - `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩·이벤트 그래프)는 보지 않았다. 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).
  - 멀티플레이 경로(원격 클라의 태그·GE·스펙 복제 통지)는 정책이 정해지지 않아(메모리 `quest-multiplayer-policy-deferred`) 깊이 보지 않았다.
  - 의도된 결정이라 뺀 것:
    - 표시된 팝업을 밖에서 닫으면 결과가 오지 않는다. `EWxPopupResult::Killed` 주석(`WxGamePopup.h:14-17`)에 명시된 계약이다.
    - `UWxConfirmationPopup::NativeOnHandleBackAction`은 `Super`를 부르지 않는다(`WxConfirmationPopup.cpp:32-39`). 엔진 기본값인 무조건 비활성화를 대체하려는 것이다.
    - `UWxActivatableWidget::GetDesiredInputConfig`는 `All`을 처리하지 않는다.
    - 뷰모델의 월드 타이머 예약은 일시정지 중에 돌지 않는다. 이펙트 게이지만 코어 티커로 남은 것도 작업 기록에 남긴 결정이다. 정지 중엔 월드 시간이 멈춰 값이 바뀌지 않으므로 통지도 나가지 않는다.
    - `UWxPrimaryGameLayout`·`UWxGamePopup` 추상 베이스 구조는 메모리 `wxui-popup-naming-convention`에 확정된 구성이다.
  - 도달 경로·가치가 없어 뺀 것:
    - `UWxUIManagerSubsystem::Initialize`의 팝업 클래스 동기 로드는 데디케이티드 서버에서도 돈다. 서버 타깃이 없다(`Source/Wx.Target.cs`·`Source/WxEditor.Target.cs`만 있음).
    - `HandlePlayerControllerSet`은 추적 PC를 먼저 비운다. 그래서 정지 위젯이 레이아웃과 함께 걷힐 때 옛 PC의 정지를 풀지 않는다. 같은 월드에서 PC를 바꾸는 코드(`SwitchController`·`SwapPlayerControllers`)가 없다.
    - `PC->SetPause`는 이미 정지 중이면 우리 `CanUnpause` 대리자를 등록하지 않는다. 다른 정지 주체가 없다(`SetPause` 호출은 서브시스템 2곳뿐).
    - 사망 화면에는 대화 창 같은 취소·회수 경로가 없다. 부활 경로가 사망 화면을 거쳐야만 열리므로(`Source/WxGame/Framework/WxRespawnLibrary.cpp:21`) 뒤늦게 뜰 창이 없다.
    - `UWxViewModel::Deinitialize` 문서 주석(`WxViewModel.h:39-45`)의 "브로드캐스트하지 않는다"는 파생 VM들의 `RF_BeginDestroyed` 가드 통지와 표현이 어긋난다. 주석 문구 문제라 `/comment-cleanup` 영역으로 보고 발견에서 뺐다.
  - 모듈 규칙은 전 파일을 확인했다.
    - `WxUI.Build.cs`·`WxUI.uplugin`은 Wx 플러그인 중 `WxCore`만 참조한다. Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다.
    - 소스 58파일과 `WxUI.Build.cs`의 첫 줄 저작권이 모두 맞다. 타입 이름은 모두 `Wx` 접두사를 쓴다.
    - 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고, 둘 다 사유 주석이 있다.

---
*문서 기준 커밋 `4096004a4` · 리뷰일 2026-09-16 · 소스 58파일 — `/module-review`로 갱신*
