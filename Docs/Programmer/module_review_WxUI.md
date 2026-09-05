# WxUI — 코드 리뷰

> 여전히 건강한 모듈이다. 비동기 push 취소, 티커 재등록 게이트, 이미지 요청 재진입, 스크린 투영 접힘 보정 같은 까다로운 지점이 의도적으로 방어돼 있고 그 이유가 주석으로 남아 있다. 이번 리뷰는 `Build.cs`·`uplugin` 의존 경계부터 서브시스템·비동기 push·뷰모델 8종·인디케이터 투영·네임플레이트·위젯·StateTree 노드까지 cpp 본문을 포함해 봤고, 직전 리뷰(`491dd7ec`)의 발견 6건은 하나도 손대지 않아 그대로 남아 있다. 새로 추가된 것은 네임플레이트 진단 누락 1건뿐이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 확인 팝업 실패 경로가 조용해 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:66-79`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:309-315`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:84-102`
- **범주**: 버그/정확성
- **문제**: `ShowConfirmation` 은 `SetBeforePushCallback` 만 걸고 `SetCompletionCallback` 은 걸지 않는다. 그래서 push 가 실패하는 모든 경로 — 레이아웃 부재(`WxAsyncAction_PushWidgetToLayer.cpp:33-37`), 로드 중 레이아웃 교체(`:100-104`), 클래스 미지정(`:24-28`) — 에서 `ResultCallback` 이 한 번도 실행되지 않고 로그도 남지 않는다. 클래스 오지정도 같다: 로드된 위젯이 `UWxGamePopup` 이 아니면 `HandleConfirmationPopupReady` 의 `Cast` 가 실패해(`:311`) `SetupPopup` 이 건너뛰어지고, 버튼이 연결되지 않은 팝업이 뜬 채 콜백도 오지 않는다. `UWxUILibrary::ShowConfirmationPopup` 은 BP 노출 API 라 결과를 기다려 흐름을 재개하는 BP 는 그대로 멈춘다.
- **제안**: `ShowConfirmation` 에서도 `SetCompletionCallback` 을 걸어, 위젯이 null 이거나 `UWxGamePopup` 캐스트가 실패하면 `ResultCallback` 을 `EWxPopupResult::Killed`(또는 별도 실패 값)로 한 번 실행하고 `LogWxUI` 경고를 남긴다.
- **확신도**: 높음 (다만 현재 `ShowConfirmationPopup` 을 부르는 C++ 도 에셋도 없고 `ConfirmationPopupClass` 도 `Config/DefaultGame.ini:13` 에 지정돼 있어, 지금 터지는 문제가 아니라 첫 사용자가 밟을 잠복 결함이다)

### 2. 🟡 PlayerController 가 교체되면 걸어 둔 게임 정지가 풀리지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:214-243` (해제 누락), `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:142-166` (`RefreshGamePause` 조기 반환)
- **범주**: 설계/구조 (상태 관리)
- **문제**: `HandlePlayerControllerSet` 은 `TrackedPlayerController.Reset()`(220행)을 **레이아웃 철거(224-228행)보다 먼저** 한다. 그래서 철거로 위젯이 비활성화되며 들어오는 통지에도 `RefreshGamePause` 는 `if (!PC) return;`(151-155행)에서 빠져나가고, 나가는 PC 에 `SetPause(false)` 를 부르는 코드는 어디에도 없다. 엔진은 `AGameModeBase::Pausers` 를 `APlayerController::SetPause(false)` → `ClearPause()` 나 `ForceClearUnpauseDelegates(PauseActor)` 로만 비우는데, 후자는 델리게이트의 UObject 가 그 액터일 때만 매칭한다 — 여기 `FCanUnpause` 의 소유자는 PC 가 아니라 서브시스템이므로 옛 PC 가 파괴돼도 항목이 남는다. 결과적으로 `bPauseGame` 위젯이 떠 있는 동안 같은 월드에서 PC 가 교체되면 월드가 정지된 채 굳는다. (맵 이동은 GameMode·WorldSettings 가 새로 만들어져 함께 사라지므로 영향이 없다.)
- **제안**: `TrackedPlayerController.Reset()` 전에 나가는 PC 에 `SetPause(false)` 를 한 번 부른다. 혹은 순서를 뒤집어 레이아웃 철거·`WatchPawnTags(nullptr)` 를 먼저 하고 그 뒤에 추적을 놓아, 철거가 일으키는 비활성화 통지가 정상 해제 경로를 타게 한다.
- **확신도**: 중간 (같은 월드 내 PC 교체 + 정지 위젯 열림이 동시에 성립해야 하는 좁은 조건이다)

### 3. 🟢 `UWxGamePopup::KillPopup` 은 호출자가 없는 데드 코드다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxConfirmationPopup.h:20`
- **범주**: 중복/복잡도
- **문제**: `KillPopup` 은 저장소 전체에서 정의·`Super::` 호출 외에 호출자가 없고 `UFUNCTION` 이 아니라 BP 에서도 부를 수 없다. 그 결과 `EWxPopupResult::Killed`(`WxGamePopup.h:15`) 도 발생하지 않는 값으로 남는다. 같은 파일의 `FWxConfirmationPopupAction::operator==`(`WxGamePopup.h:35`, 정의 `WxGamePopup.cpp:5-8`)도 사용처가 없다.
- **제안**: 발견 1 의 실패 콜백을 `KillPopup` 경로로 잇거나(그러면 `Killed` 가 의미를 되찾는다), 쓰지 않기로 하면 `KillPopup`·`Killed`·`operator==` 를 함께 걷어낸다.
- **확신도**: 높음

### 4. 🟢 `UWxViewModel_Effect::UpdateEffectState` 는 티커를 멈추면서 핸들을 비우지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:182,191,201`
- **범주**: 버그/정확성
- **문제**: 형제 격인 `UWxViewModel_Ability::UpdateCooldownState` 는 `false` 를 반환하는 자리마다 `TickerHandle.Reset()` 을 하고 그 이유를 주석으로 남겼다(`WxViewModel_Ability.cpp:394-397,412`). 이펙트 쪽은 세 곳 모두 핸들을 남긴 채 `false` 를 반환해, 이후 `TickerHandle.IsValid()` 가 "돌고 있는 티커가 있다"는 뜻이 아니게 되고 `Deinitialize` 는 이미 제거된 티커에 `RemoveTicker` 를 건다(핸들이 재사용되지 않아 오작동은 아니다). 201행(`!World`)은 실질 영향도 있다 — 한 번 걸리면 티커가 영구히 멈춰 남은 시간 표시가 언다.
- **제안**: 세 반환 지점에서 `TickerHandle.Reset()` 을 함께 한다. 201행은 반환 대신 이번 프레임만 건너뛰고 `true` 를 돌려주는 편이 의미에 맞다.
- **확신도**: 중간

### 5. 🟢 네임플레이트는 뷰모델 주입 실패를 삼킨다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:123`
- **범주**: 버그/정확성 (진단)
- **문제**: `View->SetViewModelByClass(CharacterViewModel)` 의 반환값을 버린다. 뷰모델 소스가 수동 지정(Creation Type: Manual)이 아니면 주입이 실패하는데, 위젯은 그대로 뜨고 값만 비어 보여 원인을 짚을 단서가 없다. 같은 실패를 `AWxIndicator::BindViewModel` 은 잡아 경고까지 남기고 그 이유를 주석으로 적어 뒀다(`WxIndicator.cpp:97-102`). 같은 함수 안의 다른 실패 경로(ASC 없음 `:95-99`, MVVM View 확장 없음 `:114-119`)는 이미 경고를 남기고 있어 여기만 비어 있다.
- **제안**: `WxIndicator.cpp:97-102` 와 같은 형태로 반환값을 확인해 실패 시 `LogWxUI` 경고를 남긴다.
- **확신도**: 중간 (의도적으로 생략한 것일 수 있으나, 같은 모듈·같은 함수의 다른 경로와 어긋난다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Public/` 전 헤더, 참고로 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`(사망 화면을 걷지 않는 전제 확인용)
- **확인만 하고 문제를 못 찾은 지점(기록용)**:
  - 의존 경계는 깨끗하다 — `WxUI.Build.cs` 의 Wx 의존은 `WxCore` 하나뿐이고 `uplugin` 도 같다.
  - 모든 소스 55파일의 첫 줄이 `// Copyright Woogle. All Rights Reserved.` 다(BOM 이 붙은 파일이 있으나 규칙 위반은 아니다).
  - 헤더 인라인 정의는 StateTree 두 노드의 `GetInstanceDataType()` 뿐이고, 두 헤더 모두 예외 사유 주석을 달고 있다(규칙 6 충족).
  - `UWxViewModel::FindSharedViewModel` 의 `GetObjectsWithOuter(..., EGetObjectsFlags::None, RF_NoFlags, EInternalObjectFlags::Garbage)` 는 5.8 의 신 시그니처를 정확히 쓰고 있다(구 `bool bIncludeNestedObjects` 오버로드는 deprecated). "중첩 제외 + 가비지 제외" 의도대로 동작한다.
  - 사망 화면을 걷지 않는 설계(`WxUIManagerSubsystem.cpp:286`)를 의심해 봤으나, `UWxAbility_Death` 가 종료되지 않고 저장소에 플레이어 부활 경로도 없어 현재는 문제가 되지 않는다.
  - `UWxActivatableWidget::GetDesiredInputConfig` 가 `Super::` 를 부르지 않는 것은 값 반환 가상 함수의 전면 대체라 정상이다.
  - `PushWidgetInstanceToLayerStack` 은 `UFUNCTION` 이 아니라, BP 가 정지 관찰(`ObserveWidgetForGamePause`)을 우회해 위젯을 밀어 넣을 경로는 없다.
- **미검토 / 한계**: WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이다. `AWxIndicator::UpdateProjection` 의 화면 밖 접힘·역투영 수식은 로직을 따라 읽었으나 실행 검증은 하지 않았다. 성능은 코드 판독 기준이며 프로파일링하지 않았다(네임플레이트의 프레임당 `GetFirstPlayerController`/`GetPlayerViewPoint`, 인디케이터의 프레임당 행렬 역변환은 규모상 문제로 보지 않아 발견에 넣지 않았다).

---
*문서 기준 커밋 `303d8d7f` · 리뷰일 2026-09-05 · 소스 55파일 — `/module-review`로 갱신*
