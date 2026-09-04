# WxUI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 비동기 push 취소, 티커 재등록 게이트, 이미지 요청 재진입, 스크린 투영 접힘 보정 같은 까다로운 지점이 대부분 의도적으로 방어돼 있고 그 이유가 주석으로 남아 있다. 이번 리뷰는 `Build.cs`·`uplugin` 의존 경계부터 서브시스템·비동기 push·뷰모델 7종·인디케이터 투영·위젯·StateTree 노드까지 cpp 본문을 포함해 훑었고, 발견은 정지 상태 전이·팝업 실패 경로·프로젝트 규칙 위반에 몰려 있다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 확인 팝업 실패 경로가 조용해 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:66-79`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:84-102`
- **범주**: 버그/정확성
- **문제**: `ShowConfirmation` 은 `SetBeforePushCallback` 만 걸고 `SetCompletionCallback` 은 걸지 않는다. 그래서 push 가 실패하는 모든 경로 — `ConfirmationPopupClass` 미지정(`Activate` 의 `WidgetClass.IsNull()` → `Finish(nullptr)`), 레이아웃 부재, 로드 중 레이아웃 교체 — 에서 `ResultCallback` 이 한 번도 실행되지 않는다. 로그도 남지 않는다. 같은 문제가 클래스 오지정에도 있다: 로드된 위젯이 `UWxGamePopup` 이 아니면 `HandleConfirmationPopupReady` 의 `Cast` 가 실패해(`WxUIManagerSubsystem.cpp:311`) `SetupPopup` 이 건너뛰어지고, 버튼이 연결되지 않은 팝업이 뜬 채 역시 콜백이 오지 않는다. `UWxUILibrary::ShowConfirmationPopup` 은 BP 노출 API 라, 결과를 기다려 흐름을 재개하는 BP 는 이 경우 그대로 멈춘다.
- **제안**: `ShowConfirmation` 에서도 `SetCompletionCallback` 을 걸어, 위젯이 null 이거나 `UWxGamePopup` 캐스트가 실패하면 `ResultCallback` 을 `EWxPopupResult::Killed`(또는 별도 실패 값)로 한 번 실행하고 `LogWxUI` 경고를 남긴다.
- **확신도**: 높음

### 2. 🟡 PlayerController 가 교체되면 걸어 둔 게임 정지가 풀리지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:214-243` (해제 누락), `:142-166` (`RefreshGamePause` 조기 반환)
- **범주**: 설계/구조 (상태 관리)
- **문제**: `HandlePlayerControllerSet` 은 `TrackedPlayerController.Reset()`(220행)을 **레이아웃 철거(226행)보다 먼저** 한다. 그래서 철거 중 위젯 비활성화 통지가 들어와도 `RefreshGamePause` 는 `if (!PC) return;`(151-155행)에서 빠져나가고, 나가는 PC 에 대해 `SetPause(false)` 를 부르는 코드는 어디에도 없다. 엔진은 `AGameModeBase::Pausers` 를 `APlayerController::SetPause(false)` → `ClearPause()` 나 `ForceClearUnpauseDelegates(PauseActor)` 로만 비우는데, 후자는 델리게이트의 UObject 가 그 액터일 때만 매칭한다 — 여기 `FCanUnpause` 의 소유자는 PC 가 아니라 서브시스템이므로 옛 PC 가 파괴돼도 항목이 남는다. 결과적으로 `bPauseGame` 위젯이 떠 있는 동안 같은 월드에서 PC 가 교체되면 월드가 정지된 채로 굳는다. (맵 이동은 GameMode·WorldSettings 가 새로 만들어져 함께 사라지므로 영향이 없다.)
- **제안**: `HandlePlayerControllerSet` 에서 `TrackedPlayerController.Reset()` 전에 나가는 PC 에 `SetPause(false)` 를 한 번 호출한다. 혹은 순서를 뒤집어 레이아웃 철거·`WatchPawnTags(nullptr)` 를 먼저 하고 그 뒤에 추적을 놓아, 철거가 일으키는 비활성화 통지가 정상 해제 경로를 타게 한다.
- **확신도**: 중간 (같은 월드 내 PC 교체 + 정지 위젯 열림이 동시에 성립해야 하는 좁은 조건이다)

### 3. 🟡 `BlueprintCallable` 이 BP 함수 라이브러리·Async 팩토리 밖에서 쓰인다 (코딩 규칙 5)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:65,71,74,77,80,83,86` (7건), `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20`
- **범주**: 규칙 위반
- **문제**: 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수에만 허용한다. `UWxTabListWidgetBase` 는 `UCommonTabListWidgetBase` 파생 위젯이고 `UWxButtonBase` 는 버튼 위젯이라 둘 다 해당하지 않는다. `UWxUILibrary`·`UWxMVVMConversionLibrary`(함수 라이브러리)와 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer`(팩토리)는 규칙 안이다. `UWxViewModel_Ability::TryActivateAbility`(`WxViewModel_Ability.h:40`)는 VM 커맨드로 별도 예외가 합의돼 있어 제외했다.
- **제안**: 이 API 들은 실제로 `WBP_TabList`·`WBP_TabButton`·`WBP_Button`·`WBP_Ability`·`WBP_ItemQuickSlot` 이 참조하므로 지정자를 그냥 떼면 에셋이 깨진다. 탭 리스트는 Lyra 이식본이라 실사용 표면이 좁을 가능성이 크니, WBP 그래프에서 실제로 호출되는 함수만 남기고 나머지에서 지정자를 제거하는 쪽이 현실적이다. 남기기로 한다면 규칙 문서에 "위젯 파생의 BP 노출 커맨드"를 VM 커맨드와 같은 명시적 예외로 적어 둔다.
- **확신도**: 높음 (규칙 위반 자체는 확정, 정리 범위는 WBP 실사용 확인 필요)

### 4. 🟢 `UWxGamePopup::KillPopup` 은 호출자가 없는 데드 코드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxConfirmationPopup.h:20`
- **범주**: 중복/복잡도
- **문제**: `KillPopup` 은 저장소 전체에서 정의·`Super::` 호출 외에 호출자가 없고 `UFUNCTION` 이 아니라 BP 에서도 부를 수 없다. 그 결과 `EWxPopupResult::Killed`(`WxGamePopup.h:15`) 도 발생하지 않는 값으로 남아, 이 값을 처리하는 BP 분기가 있다면 죽은 분기다. 같은 파일의 `FWxConfirmationPopupAction::operator==`(`WxGamePopup.h:35`)도 사용처가 없다.
- **제안**: 발견 1 의 실패 콜백을 `KillPopup` 경로로 잇거나(그러면 `Killed` 가 의미를 되찾는다), 쓰지 않기로 하면 `KillPopup`·`Killed`·`operator==` 를 함께 걷어낸다.
- **확신도**: 높음

### 5. 🟢 `UWxViewModel_Effect::UpdateEffectState` 는 티커를 멈추면서 핸들을 비우지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:182,191,201`
- **범주**: 버그/정확성
- **문제**: 형제 격인 `UWxViewModel_Ability::UpdateCooldownState` 는 `false` 를 반환하는 자리마다 `TickerHandle.Reset()` 을 하고 그 이유를 주석으로 남겨 뒀다(`WxViewModel_Ability.cpp:394-397,412`). 이펙트 쪽은 세 곳 모두 핸들을 남긴 채 `false` 를 반환해, 이후 `TickerHandle.IsValid()` 가 "돌고 있는 티커가 있다"는 뜻이 아니게 되고 `Deinitialize` 는 이미 제거된 티커에 `RemoveTicker` 를 건다(핸들이 재사용되지 않아 오작동은 아니다). 201행(`!World`)은 실질 영향도 있다 — 한 번 걸리면 티커가 영구히 멈춰 남은 시간 표시가 언다.
- **제안**: 세 반환 지점에서 `TickerHandle.Reset()` 을 함께 한다. 201행은 반환 대신 이번 프레임만 건너뛰고 `true` 를 돌려주는 편이 의미에 맞다.
- **확신도**: 중간

### 6. 🟢 같은 파일 안에서 동일한 조회를 람다와 루프 두 가지로 쓴다 (코딩 규칙 3)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43-47` (람다), `:64-71` (같은 조회의 루프판)
- **범주**: 규칙 위반
- **문제**: 규칙 3 은 람다를 반드시 필요한 경우로 제한한다. `GetPreregisteredTabInfo` 의 `FindByPredicate` 람다는 `TabId` 일치 탐색인데, 같은 파일 `SetTabHiddenState` 가 완전히 같은 탐색을 평범한 range-for 로 하고 있어 대체 수단이 이미 있다. 캡처 `[&]` 와 비-const 파라미터(`FWxTabDescriptor&`)도 불필요하다.
- **제안**: `GetPreregisteredTabInfo` 를 range-for 로 풀어 쓴다. (`WxUILibrary.cpp:23` 의 `CreateWeakLambda` 는 동적 델리게이트를 네이티브 델리게이트로 어댑트하는 자리라 성격이 다르니 함께 손대지 않아도 된다.)
- **확신도**: 중간 (의도된 관용구일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Public/` 전 헤더
- **확인만 하고 문제를 못 찾은 지점(기록용)**: 의존 경계는 깨끗하다 — `WxUI.Build.cs` 의 Wx 의존은 `WxCore` 하나뿐이고 `uplugin` 도 같다. `UGameInstance::Shutdown` 이 로컬 플레이어 제거 뒤에 서브시스템을 내린다는 `Deinitialize` 의 주석 전제는 엔진 5.8 소스로 확인했고 맞다. 모든 소스 첫 줄은 `// Copyright Woogle. All Rights Reserved.` 다(일부 파일에 UTF-8 BOM 이 붙어 있으나 규칙 위반은 아니다). 헤더 인라인 정의는 StateTree 두 노드의 `GetInstanceDataType()` 뿐이고 예외 사유가 주석으로 남아 있다.
- **미검토 / 한계**: WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이라, 발견 3 이 요구하는 "어떤 `BlueprintCallable` 이 실제로 호출되는가"는 확인하지 못했다 — 클래스 참조 여부만 에셋 바이너리 검색으로 확인했다. `AWxIndicator::UpdateProjection` 의 화면 밖 접힘·역투영 수식은 로직을 따라가며 읽었으나 실행 검증은 하지 않았다. 성능은 코드 판독 기준이며 프로파일링하지 않았다(네임플레이트의 프레임당 `GetFirstPlayerController`/`GetPlayerViewPoint`, 인디케이터의 프레임당 행렬 역변환은 규모상 문제로 보지 않아 발견에 넣지 않았다).

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 58파일 — `/module-review`로 갱신*
