# WxUI — 코드 리뷰

> CommonUI 레이어 스택과 MVVM 위에 얹은 UI 골격으로, 수명주기·구독 해제·재진입 처리가 전반적으로 꼼꼼하고 의도가 주석으로 잘 고정돼 있다. 이번 리뷰는 `WxUI.Build.cs`·`.uplugin`과 모든 Public 헤더를 훑고, UI 매니저·비동기 push·레이아웃·인디케이터·MVVM 뷰모델(특히 Ability/AbilitySystem/Effect)·StateTree 노드의 cpp까지 내려가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 사망 화면을 걷을 경로가 C++ 어디에도 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:233`
- **범주**: 설계/구조
- **문제**: `HandleDeathTagChanged`는 사망 화면을 push 하고 어떤 참조도 남기지 않아, 이후 C++ 어디에서도 그 위젯을 닫을 수 없다. 코드가 근거로 든 유일한 정리 경로는 240행 주석의 "PC 가 바뀌면 layout 이 통째로 재생성되면서 함께 사라진다"인데, 레이아웃 재생성은 `HandlePlayerControllerSet`(178~182행)에서 **PC 객체가 교체될 때만** 일어난다. 같은 PC 에 폰만 다시 스폰하는 리스폰(`AGameModeBase::RestartPlayer` 계열)에서는 발동하지 않으며, UI 매니저 자신이 그 경우를 위해 `HandlePossessedPawnChanged`(199행) 경로를 따로 두고 있다. 즉 사망 태그가 걷히고 폰만 갈아탄 뒤에도 Menu 레이어의 사망 화면이 그대로 남는다. 같은 파일의 대화 창은 `DialogueScreen` 추적 + `CloseDialogueScreen`(289행)으로 정확히 이 문제를 막고 있어, 사망 화면만 비대칭이다.
- **제안**: 대화 창과 같은 모양으로 맞춘다 — push 결과를 `TWeakObjectPtr`로 붙잡고, `HandleDeathTagChanged`의 `NewCount <= 0` 분기와 `WatchPawnTags`의 관찰 해제 지점에서 `DeactivateWidget()` 한다. 화면을 닫는 주체가 WBP 쪽이라면 그 계약을 주석으로 명시해 240행 설명을 정정한다.
- **확신도**: 중간 (WBP 의 버튼이 스스로 닫는 설계일 수 있음)

### 2. 🟡 `BlueprintCallable` 규칙 위반 8곳
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:39`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:65`, `:71`, `:74`, `:77`, `:80`, `:83`, `:86`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5는 `BlueprintCallable`을 Blueprint Function Library 와 Blueprint Async Action 팩토리로 제한한다. `UWxViewModel_Ability::TryActivateAbility`는 뷰모델 메서드이고, `UWxTabListWidgetBase`의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)는 위젯 메서드로 어느 쪽에도 해당하지 않는다. 같은 모듈의 `WxUILibrary.h:38`·`:41`·`:44`(BP 라이브러리)와 `WxAsyncAction_PushWidgetToLayer.h:29`(async 팩토리)는 규칙에 맞는 사용이다.
- **제안**: 뷰모델 발동은 `UWxUILibrary` 파사드 함수로 옮기거나 `BlueprintPure`/네이티브 전용으로 낮춘다. 탭 리스트의 조회 함수들은 `BlueprintPure`로, 상태 변경 함수들은 노출 필요성을 재확인해 정리한다. `WxButtonBase.h:20`의 `SetButtonText`는 위젯 서브클래스의 1-arg setter라 이 프로젝트의 허용 예외로 보고 제외했다.
- **확신도**: 높음

### 3. 🟢 티커 핸들과 실제 등록 상태가 두 뷰모델에서 서로 다르게 어긋난다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:238`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:182`
- **범주**: 버그/정확성
- **문제**: 두 파일이 정반대 방향으로 규약을 깬다.
  - `UWxViewModel_Ability::UpdateCooldownState`는 티커 콜백이면서 `SetMaxRecharges`(238행)에서 직접 호출된다. 356행의 `TickerHandle.Reset()`은 "false 반환 = 티커가 제거됨"(338~339행 주석)을 전제로 하는데, 직접 호출에서는 티커가 살아 있는 채로 핸들만 비워진다. 그 뒤 `StartCooldownTicker`의 재등록 게이트가 열려 티커가 중복 등록될 수 있고, `Deinitialize`도 그 티커를 떼지 못한다.
  - `UWxViewModel_Effect::UpdateEffectState`는 반대로 182·191·201행에서 `TickerHandle`을 비우지 않고 false 를 반환해, 이미 제거된 티커의 낡은 핸들이 남는다(현재는 재등록 경로가 없어 무해).
  실제 피해는 티커 중복 실행 정도이며 크래시로는 이어지지 않는다(`FTSTicker`가 UObject 델리게이트의 `IsBound()`로 걸러 준다).
- **제안**: 티커 콜백과 "지금 한 번 갱신"을 분리한다 — 계산 본체를 별도 함수로 빼고, 티커 콜백만 `TickerHandle` 을 만지게 한다. Effect 쪽도 false 반환 지점에서 핸들을 함께 비워 두 VM 의 규약을 같게 맞춘다.
- **확신도**: 중간

### 4. 🟢 티커에 바인딩되는 콜백 4곳에 `Handle` 접두가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:60`, `:322`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:230`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:57`
- **범주**: 규칙 위반
- **문제**: `UpdateCooldownState`·`FlushActivationRefresh`·`FlushOwnedTagsRefresh`·`UpdateEffectState`는 모두 `FTickerDelegate::CreateUObject`로 바인딩되는 콜백인데 CLAUDE.md 코딩 규칙 4의 `Handle` 접두가 없다. 모듈 내 다른 델리게이트 콜백(`HandleTagChanged`·`HandleActiveEffectAdded` 등)은 전부 규칙을 지키고 있어 이 4개만 예외다. 저장소 전체에서 `FTickerDelegate` 바인딩은 이 4곳뿐이라 참고할 다른 관례도 없다.
- **제안**: 규칙대로 접두를 붙이거나(`HandleCooldownTick` 등), 티커를 델리게이트 콜백 규칙의 예외로 볼 것인지 CLAUDE.md 에 한 줄로 못 박는다.
- **확신도**: 높음

### 5. 🟢 `RefreshGamePause`가 UI 와 무관한 정지까지 해제한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:150`
- **범주**: 설계/구조
- **문제**: 위젯 활성/비활성이 바뀔 때마다 `UGameplayStatics::SetGamePaused(World, bWantsPause)`를 무조건 호출한다. `bWantsPause == false`면 `AGameModeBase::ClearPause()`가 돌아 **다른 주체가 건 정지까지 전부 지운다**. 지금은 저장소 전체에서 `SetGamePaused` 호출이 이 한 줄뿐이라 충돌하지 않지만, 연출·디버그 등 다른 정지 소스가 생기는 순간 인벤토리 여닫기 같은 무관한 UI 조작이 그 정지를 풀어 버린다.
- **제안**: 이 서브시스템 전용 pauser 이름으로 `PC->SetPause`/`ClearPause` 짝을 쓰거나, 최소한 자신이 걸었던 정지일 때만 해제하도록 좁힌다.
- **확신도**: 중간 (v1 단일 정지 소스 전제라면 의도된 단순화)

### 6. 🟢 `UWxActionWidget` 데드 코드
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxActionWidget.h:9`
- **범주**: 중복/복잡도
- **문제**: 본문이 비어 있고 `// TODO: 삭제 예정` 만 달린 클래스로, 모듈 안에서 참조하는 곳이 없다.
- **제안**: BP 에셋이 이 클래스를 상속하고 있지 않은지 확인한 뒤 제거한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`
- **훑은 파일**: 나머지 Public 헤더 전부와 `Private/MVVM/WxViewModel_Indicator.cpp`, `Private/MVVM/WxViewModel_Subtitle.cpp`, `Private/MVVM/WxViewModel_Character.cpp`, `Private/MVVM/WxViewModel_Interaction.cpp`, `Private/MVVM/WxMVVMConversionLibrary.cpp`, `Private/Widget/WxConfirmationPopup.cpp`, `Private/Widget/WxGamePopup.cpp`, `Private/Widget/WxHUDLayout.cpp`, `Private/Widget/WxButtonBase.cpp`, `Private/Widget/WxTabButtonBase.cpp`, `Private/Widget/WxActivatableWidget.cpp`, `Private/Component/WxNameplateComponent.cpp`, `Private/Indicator/WxIndicatorDescriptor.cpp`, `Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`
- **확인했고 문제 없던 것**: 모듈 의존성은 `WxCore` + 엔진/플러그인뿐이라 도메인 간 참조 금지 규칙을 지킨다. 전 소스 첫 줄 Copyright 존재, `FORCEINLINE`·헤더 인라인 정의 없음(StateTree `GetInstanceDataType()` 2곳은 예외 사유 주석이 달려 있음), `WxViewModel`/`WxViewModel_Ability` 등의 구독·티커 해제 짝, `WxAsyncAction_PushWidgetToLayer`의 동기 완료·취소 재진입 처리, `FindSharedViewModel`의 `GetObjectsWithOuter` 인자(`EGetObjectsFlags::None` = 중첩 제외)와 Garbage 제외, `ProjectIndicator`의 `ULocalPlayer::GetPixelPoint` 사용(슬레이트 단위 `FVector2f*` 전달이 UE 5.8 시그니처와 일치), `UWxTabListWidgetBase::NativeDestruct` → 엔진 `RemoveAllTabs` 순서.
- **미검토 / 한계**: 레이어 z-order·바인딩 이름 등 WBP 에셋 쪽 계약(범위 밖)과 실제 런타임 동작은 확인하지 않았다. 사망/대화/HUD 위젯이 스스로 닫는지 여부는 WBP 내부라 발견 1의 확신도를 중간에 둔 이유다. 스플릿스크린은 코드가 단수 로컬 플레이어를 명시적 전제로 두고 있어(`WxUIManagerSubsystem.h:76~79`) 별도로 파고들지 않았다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 60파일 — `/module-review`로 갱신*
