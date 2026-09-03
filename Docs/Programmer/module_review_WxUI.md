# WxUI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 수명주기·구독 해제·재진입·GC 참조 흐름이 대체로 정확히 다뤄져 있고, 위험한 지점마다 근거 주석이 붙어 있다(티커 재등록 게이트, 스트리밍 재요청 경합, MVVM 공유본 해제 금지 등). 의존성도 `WxCore` 만 참조해 모듈 규칙을 지킨다. 이번 리뷰는 `WxUI.Build.cs`/`WxUI.uplugin`, 전체 Public 헤더, 그리고 매니저·async push·MVVM(ViewModel 베이스/AbilitySystem/Ability/Effect/Attribute)·인디케이터·자막 ST 노드·팝업/탭 위젯의 cpp 구현까지 내려가 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 팝업이 버튼 외 경로로 닫히면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:86`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:81`
- **범주**: 버그/정확성
- **문제**: 결과 콜백은 `HandleResultChosen` 한 곳에서만 실행되고, 그 진입점은 버튼 3개의 `OnClicked` 와 `KillPopup()` 뿐이다. 그런데 `KillPopup()` 은 전 코드베이스 어디서도 호출되지 않으며(정의·override 뿐), 그 결과 `EWxPopupResult::Killed`(`Public/Widget/WxGamePopup.h:15`)는 도달 불가 값이다. 반면 팝업을 버튼 없이 닫는 경로는 실재한다 — CommonUI Back 입력으로 스택 top 이 비활성화되는 경우, `UWxUILibrary::DeactivateWidgetsInLayer` 가 Modal 레이어에 `ClearWidgets()` 를 거는 경우, PC 교체로 layout 이 통째로 재생성되는 경우(`WxUIManagerSubsystem.cpp:200-204`). 이 경로들에서 `ShowConfirmation` 호출자는 어떤 결과도 받지 못한 채 무한 대기한다. 현재 `ShowConfirmationPopup` 을 부르는 C++·에셋 소비자가 아직 없어 증상이 드러나지 않았을 뿐이다.
- **제안**: `UWxConfirmationPopup` 의 `NativeOnDeactivated`(또는 `NativeDestruct`)에서 콜백이 아직 바인드돼 있으면 `KillPopup()` 을 태워 `Killed` 를 내보낸다. 그러면 언바인딩이 이미 중복 실행 가드 역할을 하므로 추가 상태가 필요 없다.
- **확신도**: 높음(사실 관계). 다만 소비자가 아직 없어 현재 영향은 잠재적이다.

### 2. 🟡 사망 화면은 띄우기만 하고 걷는 주체가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:255`
- **범주**: 설계/구조
- **문제**: `HandleDeathTagChanged` 는 태그가 붙을 때만 push 하고, 걷힐 때(`NewCount <= 0`)는 아무것도 하지 않으며 push 액션·위젯 참조를 하나도 남기지 않는다. 바로 아래 `HandleDialogueTagChanged`(`:268`)가 `PendingDialogueScreenPush` + `DialogueScreen` 을 들고 태그 해제·폰 교체 양쪽에서 대칭으로 걷어내는 것과 대조된다. 코드 주석은 "PC 가 바뀌면 layout 이 재생성되면서 함께 사라진다"에 기대는데, 이 프로젝트의 리스폰은 `AWxGameMode::RestartPlayer`(`Source/WxGame/Framework/WxGameMode.cpp:115`)로 **같은 PlayerController** 에 새 폰을 붙이는 경로라 `HandlePlayerControllerSet` 이 돌지 않고 layout 도 재생성되지 않는다. 즉 리스폰/부활이 붙는 순간 (a) 사망 화면이 Menu 레이어에 남고, (b) 두 번째 사망 때 화면이 그 위에 겹쳐 쌓인다. 폰만 갈아타는 흐름(탈것·연출 폰)에서도 매니저가 죽은 폰의 ASC 관찰을 놓아버려 화면을 되돌릴 수단이 사라진다.
- **제안**: 대화 화면과 같은 모양으로 `PendingDeathScreenPush`/`DeathScreen` 을 들고, `NewCount <= 0` 과 `WatchPawnTags` 진입부에서 함께 걷는다. 지금 화면 해제를 WBP 가 맡고 있다면, 최소한 재진입 시 중복 push 를 막는 가드만이라도 둔다.
- **확신도**: 중간(현재 C++ 리스폰 경로가 없어 아직 재현되지 않는다 — 의도된 유예일 수 있음)

### 3. 🟡 규칙 5 위반: 위젯·뷰모델에 `BlueprintCallable` 8건
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:65,71,74,77,80,83,86`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:40`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리로 제한한다. `UWxTabListWidgetBase` 는 `GetPreregisteredTabInfo`/`SetTabHiddenState`/`RegisterDynamicTab`/`IsFirstTabActive`/`IsLastTabActive`/`IsTabVisible`/`GetVisibleTabCount` 7개를 노출하고(Lyra 계열 이식 흔적), `UWxViewModel_Ability::TryActivateAbility` 는 뷰모델에서 어빌리티를 발동시킨다. 같은 모듈의 `UWxUILibrary`·`UWxMVVMConversionLibrary`·`UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer` 는 규칙에 맞는 형태라 대비가 뚜렷하다. (`UWxButtonBase::SetButtonText`(`Public/Widget/WxButtonBase.h:20`)는 위젯 서브클래스의 1-arg setter라 허용 예외 범주로 보고 제외했다.)
- **제안**: 탭 리스트의 조회·조작 API 는 `UWxUILibrary`(또는 전용 탭 라이브러리)로 옮겨 위젯 인스턴스를 인자로 받게 하고, `TryActivateAbility` 도 어빌리티 발동 파사드를 통하게 한다. `WBP_TabList`(`Content/UI/Widget/WBP_TabList.uasset`)가 실제로 이 노드들을 쓰고 있는지 먼저 확인한 뒤 옮긴다.
- **확신도**: 높음

### 4. 🟢 `UWxViewModel_Effect::UpdateEffectState` 가 false 반환 시 `TickerHandle` 을 비우지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:177`
- **범주**: 버그/정확성(잠재)
- **문제**: 세 곳(`:182`, `:191`, `:201`)에서 `false` 로 티커를 스스로 제거하면서 `TickerHandle` 은 그대로 둔다. 형제 클래스는 같은 자리에서 `TickerHandle.Reset()` 을 하고 그 이유를 명시했다 — "남겨 두면 재등록 게이트가 닫힌 채로 굳어 갱신이 영구 정지한다"(`Private/MVVM/WxViewModel_Ability.cpp:394`). 이펙트 VM 은 티커를 `Initialize` 에서 한 번만 다는 구조라 지금은 증상이 없지만(핸들도 다음 프레임엔 만료된다), 나중에 재적용·스택 갱신으로 티커를 다시 다는 코드가 붙는 순간 그대로 동결 버그가 된다.
- **제안**: `false` 를 반환하는 세 경로에서 `TickerHandle.Reset()` 을 함께 한다.
- **확신도**: 중간

### 5. 🟢 규칙 4 위반: 티커 델리게이트 콜백에 `Handle` 접두 없음
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:40,378`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:221,234`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:57`
- **범주**: 규칙 위반
- **문제**: `FTickerDelegate::CreateUObject` 에 바인딩되는 `UpdateCooldownState`·`FlushActivationRefresh`·`FlushOwnedTagsRefresh`·`FlushAbilityRebind`·`UpdateEffectState` 가 규칙 4 의 `Handle` 접두를 따르지 않는다. 같은 파일들의 ASC 델리게이트 콜백(`HandleTagChanged`, `HandleGameplayEffectApplied`, `HandleCostAttributeChanged`)은 규칙을 지키고 있어 명명 기준이 파일 안에서 갈린다.
- **제안**: `HandleCooldownTick`·`HandleActivationRefreshFlush` 처럼 접두를 맞추거나, 프레임 합치기 티커는 예외로 둔다는 판단을 규칙 쪽에 한 줄 남긴다.
- **확신도**: 중간(의도된 명명 구분일 수 있음)

### 6. 🟢 같은 탭 조회를 람다와 루프로 두 번 구현
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43`, 같은 파일 `:62`
- **범주**: 중복/복잡도 · 규칙 위반
- **문제**: `GetPreregisteredTabInfo` 는 `FindByPredicate` + 람다로, `SetTabHiddenState` 는 명시 루프로 `TabId` 가 같은 항목을 찾는다. 동일 탐색의 이중 구현이고, 람다 쪽은 `CLAUDE.md` 규칙 3(람다는 반드시 필요할 때만)에도 걸린다 — 모듈의 다른 탐색은 전부 명시 루프다.
- **제안**: `FWxTabDescriptor* FindTabInfo(FName)` 하나로 합치고 두 함수가 공유한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, Public 헤더 전량
- **미검토 / 한계**:
  - WBP/WBP-그래프(`WBP_PrimaryGameLayout`, `WBP_DeathScreen`, `WBP_DialogueScreen`, `WBP_TabList` 등)는 리뷰 범위 밖이라, 발견 1·2 가 실제로 콘텐츠 쪽에서 이미 보완돼 있는지는 확인하지 못했다.
  - `AWxIndicator::UpdateProjection` 의 화면 밖 클램프·역투영 수학은 코드 흐름과 좌표계 전제만 따라갔고, 레터박스/울트라와이드/스플릿뷰에서의 실제 화면 검증은 하지 않았다.
  - 멀티플레이 경로: 게임 정지는 `NM_Standalone` 로 스스로 제한하고 인디케이터·자막도 v1 싱글/리슨 전제를 주석으로 명시하고 있어, 리슨 서버 클라이언트에서의 실동작은 확인 대상에서 뺐다.
  - 성능은 코드 상 명백한 것(매 프레임 활성 GE 순회, 인디케이터 틱당 행렬 역산)만 봤고 프로파일은 돌리지 않았다. 둘 다 근거 주석이 있는 의도된 선택으로 판단해 발견으로 올리지 않았다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 58파일 — `/module-review`로 갱신*
