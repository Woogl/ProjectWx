# WxUI — 코드 리뷰

> 모듈은 건강하다. 레이어 push·정지 재평가·뷰모델 공유와 해제 규약이 일관되게 지켜지고, 심각하거나 개선이 필요한 결함은 찾지 못했으며 남은 발견은 모두 사소한 정리 항목이다. 커버리지: 소스 58파일을 모두 읽었다. 제어 흐름의 중심(서브시스템·push 액션·HUD·레이아웃 컴포넌트·팝업)과 뷰모델 수명 경로는 엔진·Lyra 원본과 대조해 정독했고, 지난 리뷰 이후 바뀐 `WxViewModel_Subtitle.cpp`·`WxIndicator.h`도 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟢 레이아웃 컴포넌트의 "같은 폰" 게이트에 도달하지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp:54-69`, `:44-48`
- **범주**: 중복/복잡도
- **문제**:
  - `:44-48`은 옛 폰과 새 폰이 다르면 먼저 `ClearLayout`을 부른다. `ClearLayout`은 `PendingLayoutPush`와 `LayoutWidget`을 비운다(`:85-89`, `:100`). 그래서 `:60-69`의 두 게이트는 두 폰이 같은 알림에서만 반환할 수 있다.
  - 그런 알림은 오지 않는다.
    - 엔진은 폰이 실제로 바뀔 때만 `OnPossessedPawnChanged`를 방송한다. `Possess`(엔진 `Controller.cpp:346-350`), `UnPossess`(`:400-404`), `OnRep_Pawn`(`:570-572`)이 모두 그렇다.
    - `Possess`의 알림 강제 분기는 옛 폰을 nullptr로 보낸다(`:350`).
    - BeginPlay 따라잡기(`WxPlayerLayoutComponent.cpp:27`)도 옛 폰을 nullptr로 넘긴다.
  - 결국 두 게이트는 이미 비운 멤버만 검사한다(`Contains(nullptr)`, null `PendingLayoutPush`). `:60` 주석("같은 Pawn 알림은 기존 HUD를 유지한다")은 일어나지 않는 경우를 설명한다.
  - 게이트는 폰을 갈아타도 HUD를 유지하던 설계(`86b134d6`, 08-28)에서 왔고, 폰 교체 때 HUD를 걷는 분기가 앞에 붙으면서(`9a03745d`, 09-06) 역할을 잃었다.
- **제안**: `:60-69`를 걷는다. 그러면 `:54-58`의 레이아웃 조회도 게이트 말고는 쓰는 곳이 없어 함께 걷을 수 있다. 레이아웃이 없으면 푸시 액션이 실패 완료로 끝낸다(`WxAsyncAction_PushWidgetToLayer.cpp:30-37`).
- **확신도**: 높음

### 2. 🟢 `SetBeforePushCallback`이 호출자 없는 데드 코드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h:34-35`, `:61`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:62-65`, `:120`, `:161-162`
- **범주**: 중복/복잡도
- **문제**:
  - 저장소 C++(`Source/`·`Plugins/`)에 `SetBeforePushCallback` 호출이 없다. `UFUNCTION`이 아니라 BP 경로도 없다. 그래서 선언·멤버·실행 줄(`:120`)·언바인딩(`:162`)이 아무 일도 하지 않는다.
  - `:161` 주석("콜백 페이로드가 붙잡고 있는 참조를 여기서 놓는다")은 이미 사라진 서술자 페이로드를 설명한다. 남은 완료 콜백 3곳은 모두 페이로드 없는 `CreateUObject`다(`WxHUDLayout.cpp:88-89`, `WxPlayerLayoutComponent.cpp:72-73`, `WxUIManagerSubsystem.cpp:312-313`).
- **제안**: `SetBeforePushCallback` 선언·정의, `BeforePushCallback` 멤버, `:120` 실행 줄, `:161-162` 주석과 언바인딩을 걷는다. BP 비동기 노드용 `BeforePush` 동적 델리게이트는 그대로 둔다.
- **확신도**: 높음

### 3. 🟢 단일 호출 헬퍼 `MakeNativeResultDelegate`가 익명 namespace에 남아 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`, `:83`
- **범주**: 중복/복잡도
- **문제**:
  - 호출부는 `UWxUILibrary::ShowConfirmationPopup` 한 곳뿐이다(`:83`). 모듈에 남은 유일한 `namespace { }` 블록이다.
  - 사용자는 `.cpp` 내부 헬퍼를 익명 namespace 대신 호출부에 인라인하는 쪽을 택해 왔다. 같은 모양의 `WxViewModel_Subtitle.cpp` 헬퍼도 지난 리뷰 뒤 인라인했다(`08218a58`).
  - 지난 리뷰의 "람다를 `CreateUFunction`으로 바꾸라"는 제안은 CLAUDE.md에서 람다 제한이 빠져(`5fe1ceb6`) 더는 근거가 없다. 남는 것은 헬퍼 위치 문제뿐이다.
- **제안**: `:83` 직전에 `FWxPopupResultDelegate` 지역 변수를 두고, `OnResult.IsBound()`일 때만 지금의 `CreateWeakLambda`를 대입해 넘긴다. 그다음 namespace 블록을 걷는다.
- **확신도**: 중간(CLAUDE.md 명문 규칙이 아니라 사용자 선호로 정한 방침이다)

### 4. 🟢 인라인 예외 주석이 없어진 "코딩 규칙 4"를 가리킨다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Indicator/WxStateTreeTask_MarkIndicator.h:15`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxStateTreeTask_PrintSubtitle.h:13`
- **범주**: 규칙 위반
- **문제**:
  - CLAUDE.md는 `GetInstanceDataType()` 헤더 정의를 예외로 허용하되 그 자리에 사유 주석을 남기라고 한다. 두 헤더의 사유 주석은 있고 내용도 맞다.
  - 다만 `5fe1ceb6`에서 람다 규칙(구 3번)이 빠지면서 인라인 금지가 3번으로 올라왔다. 두 주석은 여전히 "코딩 규칙 4의 예외"라고 적어, 규칙 준수를 확인하는 사람이 존재하지 않는 번호를 찾게 된다.
  - 같은 문장이 다른 모듈 헤더 25곳(WxDialogue·WxInventory·WxQuest·WxWorld)에도 있다.
- **제안**: 두 주석의 "코딩 규칙 4"를 "코딩 규칙 3"으로 고친다. 다른 모듈도 한 번에 일괄 치환하는 편이 낫다(`grep -rn "코딩 규칙 4" Plugins`).
- **확신도**: 높음

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
  - 뷰모델·인디케이터·자막:
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`
    - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`(이번 변경분 포함)
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h`(이번 변경분 포함)
    - `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`
  - 판정 근거로 본 외부 코드:
    - UE 5.8 `Engine/Private/Controller.cpp`: `Possess`·`UnPossess`·`OnRep_Pawn`의 빙의 알림 조건
    - UE 5.8 `Engine/Private/LocalPlayer.cpp`: `ReceivedPlayerController`의 `OnPlayerControllerChanged` 방송
    - UE 5.8 `Engine/Private/LevelTick.cpp`: 일시정지 중 월드 타이머 미실행
    - UE 5.8 `UMG/Private/Components/WidgetComponent.cpp`: `InitWidget`이 `SetWidget`을 거치지 않음(네임플레이트 이중 바인딩 없음 확인)
    - Lyra 5.7 `LyraHUDLayout.cpp`, CommonGame `PrimaryGameLayout.h`·`CommonUIExtensions.cpp`: 스트리밍 push 핸들 처리
  - 작업 기록: `.claude/worklog/2026-08-03-HUD-자막.md`, `.claude/worklog/2026-09-14-코드리뷰-반영-월드타이머-전환.md`, `.claude/worklog/2026-09-15-가려진-확인-팝업-종료-판정.md`, `.claude/worklog/2026-09-16-Subtitle-익명-namespace-헬퍼-제거.md`, `.codex/worklog/2026-09-15-Ability-쿨다운-월드타이머.md`
- **훑은 파일**:
  - `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `WxViewModel_Item.cpp`, `WxViewModel_Interaction.cpp`, `WxViewModel_Indicator.cpp`, `WxViewModelResolver_PlayerCharacter.cpp`, `WxMVVMConversionLibrary.cpp`
  - `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`
  - `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `WxButtonBase.cpp`
  - `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`
  - `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`
  - `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩·이벤트 그래프)는 보지 않았다. 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).
  - 멀티플레이 경로(원격 클라의 태그·GE·스펙 복제 통지)는 v1 싱글/리슨 호스트 전제라 깊이 보지 않았다.
  - 지난 리뷰 대비 정리:
    - 지난 1번(HUD가 가려지거나 걷혀도 진행 중인 메뉴 요청을 취소하지 않음)은 뺐다. 코드는 그대로지만 Lyra HUD도 스트리밍 push 핸들을 버리고 비활성화 때 취소하지 않는다(`LyraHUDLayout.cpp:67-71`). Lyra와 같은 패턴이라 결함으로 보지 않는다. 재현 창도 메뉴 클래스를 처음 스트리밍하는 동안뿐이다.
    - 지난 4번(람다)은 CLAUDE.md에서 람다 제한이 빠져 규칙 위반 근거가 없어졌다. 남은 익명 namespace 문제만 3번으로 옮겼다.
    - 지난 5번(`MakeSubtitleContext`)은 `08218a58`에서 해결됐다.
    - `WxIndicator.h` 변경은 `HasTarget` 주석 축약뿐이다. 유일한 호출부(`WxStateTreeTask_MarkIndicator.cpp:109`)와 의미가 맞는다.
  - 의도된 결정이라 뺀 것:
    - 표시된 팝업을 밖에서 닫으면 결과가 오지 않는다. `EWxPopupResult::Killed` 주석(`WxGamePopup.h:14-17`)에 명시된 계약이며, 결과를 사용자 선택에만 묶는 Lyra 확인 창 방식이다.
    - `UWxConfirmationPopup::NativeOnHandleBackAction`은 `Super`를 부르지 않는다(`WxConfirmationPopup.cpp:32-39`). 엔진 기본 동작인 무조건 비활성화를 대체하려는 것이다.
    - `UWxActivatableWidget::GetDesiredInputConfig`는 `All`을 처리하지 않는다.
    - 뷰모델의 월드 타이머 예약(태그·스펙 재평가, 쿨다운 갱신)은 일시정지 중에 돌지 않는다. 작업 기록에 감수 사항으로 남아 있다. 이펙트 게이지만 코어 티커다.
    - 자막 뷰모델은 UI 매니저가 소유하지 않고 스스로 MVVM 글로벌 컬렉션에 등록한다(`WxViewModel_Subtitle.cpp:12-37`). 별도 서브시스템을 만들지 않았고, 매니저 관여를 걷어낸 결정이 `2026-08-03-HUD-자막.md` 완료 항목에 있다.
    - 이펙트·어빌리티 VM은 인스턴스마다 자기 티커·타이머를 건다. 자기완결 구동이라 방침에 맞고, 비용 문제도 관찰되지 않았다.
  - 도달 경로·가치가 없어 뺀 것:
    - `UWxUIManagerSubsystem::Initialize`의 팝업 클래스 동기 로드는 데디케이티드 서버에서도 돈다. 하지만 서버 타깃이 없다(`Source/Wx.Target.cs`·`Source/WxEditor.Target.cs`만 있음).
    - `HandlePlayerControllerSet`은 추적 PC를 먼저 비운다. 그래서 정지 위젯이 레이아웃과 함께 걷힐 때 옛 PC의 정지를 풀지 않는다. 하지만 같은 월드에서 PC를 바꾸는 코드(`SwitchController`·`SwapPlayerControllers`)가 없다.
    - 사망 화면에는 대화 창 같은 취소·회수 경로가 없다. 부활 요청이 활성 사망 화면을 요구하므로(`Source/WxGame/Framework/WxRespawnLibrary.cpp:21`) 뒤늦게 뜰 창이 없다.
    - 주석·표기 수준이라 `/comment-cleanup` 영역으로 보고 뺀 것이 2건 있다.
      - `ObserveWidgetForGamePause`의 "CommonUI 풀에서 재사용" 주석(`WxUIManagerSubsystem.cpp:135`)은 실제와 다르다. push 경로는 새로 만든 인스턴스를 `AddWidgetInstance`로 넣으므로 CommonUI 풀을 거치지 않는다(엔진 `CommonActivatableWidgetContainer.cpp:38-41`, `UserWidgetPool.cpp:44-48`). 구독을 뗐다 다시 거는 코드 자체는 무해하다.
      - `UWxViewModel_Indicator::SetProjection`의 인자 이름이 선언(`InDistanceMeters`)과 정의(`InCameraDistance`)에서 다르다.
  - 모듈 규칙은 전 파일을 확인했다.
    - `WxUI.Build.cs`·`WxUI.uplugin`은 Wx 플러그인 중 `WxCore`만 참조한다. Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다.
    - 소스 58파일과 `WxUI.Build.cs`의 첫 줄 저작권이 모두 맞다. 타입 이름은 모두 `Wx` 접두사를 쓴다.
    - 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고, 둘 다 사유 주석이 있다(번호만 틀림, 4번 참고). `FORCEINLINE`은 없다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 58파일 — `/module-review`로 갱신*
