# WxUI — 코드 리뷰

> 수명주기 관리(구독 해제, 스트리밍 취소, 늦은 도착 규약)가 전반적으로 꼼꼼하고 의도 주석도 잘 붙어 있어 건강한 편이다. 명백한 널 역참조·댕글링·모듈 경계 침범은 발견되지 않았고, 남은 것은 레퍼런스에서 통째로 들여온 미사용 API와 결과 콜백이 조용히 유실될 수 있는 경로다. 이번 리뷰는 `Build.cs`·`uplugin` 의존성, 시스템/비동기 push/뷰모델/인디케이터/팝업 계열의 cpp 본문까지 내려가 봤고, 위젯 에셋(WBP) 내부는 범위 밖이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 확인 팝업 결과 콜백이 버튼 외의 경로로 닫히면 조용히 유실된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:86`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91`
- **범주**: 버그/정확성
- **문제**: `OnResultCallback` 은 `HandleResultChosen` 에서만 실행되고, 그 진입점은 `NativeOnInitialized` 가 건 버튼 3종(`WxConfirmationPopup.cpp:12-23`)과 `KillPopup()` 뿐이다. 그런데 `KillPopup()` 은 저장소 전체에 호출자가 없고, `UFUNCTION` 도 아니라 BP 에서도 부를 수 없다(`WxGamePopup.h:70`). 즉 버튼을 누르지 않고 창이 닫히는 경로 — WBP 가 CommonUI Back 핸들러를 켠 경우, `UWxUILibrary::DeactivateOwningActivatable`/`DeactivateWidgetsInLayer`(`WxUILibrary.cpp:50,67`), 컨트롤러 교체로 레이아웃이 통째로 재생성되는 경우(`WxUIManagerSubsystem.cpp:188-192`) — 에서는 요청자가 결과를 영영 못 받는다. `EWxPopupResult::Killed` 는 그래서 발행 지점이 없는 값이다. 지금은 `ShowConfirmationPopup` 자체에 호출자가 없어 잠재 상태지만, 첫 사용자가 그대로 밟는다.
- **제안**: `UWxConfirmationPopup::NativeOnDeactivated` 를 오버라이드해 콜백이 아직 바인딩돼 있으면 `HandleResultChosen(EWxPopupResult::Killed)` 로 흘려보낸다. 그러면 `KillPopup()` 의 죽은 가상 함수도 함께 정리된다.
- **확신도**: 높음(메커니즘) / 영향은 현재 잠재

### 2. 🟡 탭 리스트에 레퍼런스에서 들여온 미사용 API가 그대로 남아 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:68-94`
- **범주**: 중복/복잡도
- **문제**: `GetAllPreregisteredTabInfos`(h:68) `SetTabHiddenState`(h:72) `RegisterDynamicTab`(h:75) `IsFirstTabActive`(h:78) `IsLastTabActive`(h:81) `IsTabVisible`(h:84) `GetVisibleTabCount`(h:87) `OnTabContentCreated`/`OnTabContentCreatedNative`(h:92-94) 는 C++ 어디서도 호출되지 않고, `Content/{UI,AI,AbilitySystem,Character,Framework,Item,Quest,Input,DesignerTables}` 에셋의 이름 테이블에도 하나도 등장하지 않는다(정적 클래스 참조는 1건 있어 베이스 자체는 쓰인다). `RegisterDynamicTab` 이 유일한 기입자이므로 `PendingTabLabelInfoMap`(h:124)은 항상 비고, `HandleTabCreation_Implementation` 의 폴백 분기(`WxTabListWidgetBase.cpp:125`)도 따라서 죽은 코드다. 같은 성격으로 `UWxTabButtonBase::SetIconFromLazyObject`(`WxTabButtonBase.cpp:8`)와 `FWxConfirmationPopupAction::operator==`(`WxGamePopup.cpp:5`)도 호출자가 없다. 프로젝트 방침(레퍼런스는 핵심만 채택)과 어긋나고, 읽는 쪽은 어디까지가 실제로 도는 코드인지 매번 다시 확인해야 한다.
- **제안**: 실제로 도는 경로(`SetupTabs` → `RegisterTab` → `HandleTabCreation`)만 남기고 나머지를 걷어낸다. 나중에 필요해지면 그때 되살리는 편이 싸다.
- **확신도**: 중간(BP 그래프가 이름으로 호출할 여지는 에셋 grep 으로 배제했으나, 미검색 콘텐츠 폴더가 남아 있음)

### 3. 🟡 `TryActivateAbility` 가 `Request~` 명명 예외를 벗어난 `BlueprintCallable` 이다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:39`
- **범주**: 규칙 위반
- **문제**: 규칙 5 는 `BlueprintCallable` 을 BP Function Library 와 Async Action 팩토리로 제한하고, 이 프로젝트가 인정한 유일한 확장이 뷰모델의 Command 계열 `Request~` 함수다. `WxViewModel_Dialogue::RequestAdvance`, `WxViewModel_InteractionList::RequestInteract/RequestCycle`, `WxViewModel_Item::RequestUseConsumable` 이 모두 그 규약을 따르는데 이 함수만 벗어나 있어, 예외에 해당하는지 이름으로 판별할 수 없다.
- **제안**: `RequestActivate` 로 개명한다. 호출하는 위젯 에셋이 1건 있으므로 바인딩 재지정이 함께 필요하다.
- **확신도**: 중간

### 4. 🟡 UI 매니저가 레이어 기구(mechanism)와 콘텐츠 정책(사망·대화 화면)을 함께 쥐고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:243`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:256`
- **범주**: 설계/구조
- **문제**: 이 서브시스템 하나가 레이아웃 수명주기·PC 추적·폰 ASC 태그 관찰·사망 화면·대화 창·확인 팝업·게임 정지 재평가를 모두 맡는다. 앞의 셋은 UI 기구지만 뒤의 둘은 "사망하면 이 화면", "대화가 열리면 저 화면" 이라는 게임 콘텐츠 정책이고, 그래서 `UWxUIDeveloperSettings` 에도 `DeathScreenClass`/`DialogueScreenClass`(`WxUIDeveloperSettings.h:29,33`)라는 도메인 색이 밴 항목이 생겼다. HUD 는 이미 Experience 가 `SetGameHUDClass` 로 발행하는 반대 모양이라 두 방식이 한 클래스 안에 섞여 있다. 다만 관찰을 도메인 델리게이트가 아니라 WxCore 태그로 듣는 선택 자체는 DAG 를 지키려는 의도된 설계이며(코드 주석에 근거가 남아 있다), 플러그인 참조 위반은 아니다.
- **제안**: 새 화면을 추가할 때마다 이 클래스가 커지는 것이 부담이 되면, 태그→화면 매핑을 WxGame 쪽 조율 컴포넌트로 옮기고 WxUI 에는 push/정지 기구만 남긴다. 지금 당장 깨지는 것은 없으므로 다음 화면 추가 시점의 판단 재료로 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 티커 핸들 기록이 실제 등록 상태와 어긋날 수 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:238`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:177`
- **범주**: 버그/정확성
- **문제**: `UpdateCooldownState` 는 티커 콜백이면서 `SetMaxRecharges` 에서 직접 호출되기도 한다(`:238`). 직접 호출 경로에서 쿨다운이 없다고 판정되면 `TickerHandle.Reset()`(`:357`)이 도는데, 이때 등록된 티커는 그대로 살아 있다. 재등록 게이트가 `!TickerHandle.IsValid()` 라서, 그 프레임 안에 쿨다운 GE 가 새로 적용되면 티커가 둘 등록되고 그중 하나는 `Deinitialize` 가 회수하지 못한다. 세터가 모두 멱등이라 표시가 틀어지지는 않고 다음 판정에서 스스로 풀리지만, 소유 관계가 한 함수 안에서 두 갈래로 갈려 있는 것이 원인이다. `UWxViewModel_Effect::UpdateEffectState` 는 반대로 false 를 반환하면서 핸들을 비우지 않아(`:181,191,201`), 같은 클래스에 재등록 경로가 생기는 순간 형제 클래스가 주석으로 경고해 둔 "게이트가 닫힌 채 굳는" 상태에 빠진다.
- **제안**: 등록·해제 기록을 `StartCooldownTicker`/`Deinitialize` 한 쌍에만 맡기고, 직접 호출용 갱신은 티커 콜백과 분리한다. Effect 쪽은 false 반환 지점에서 핸들을 함께 비워 형제와 규약을 맞춘다.
- **확신도**: 중간

### 6. 🟢 티커에 바인딩되는 콜백이 `Handle` 접두를 쓰지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:59`, `:321`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:56`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:229`
- **범주**: 규칙 위반
- **문제**: `UpdateCooldownState`·`FlushActivationRefresh`·`UpdateEffectState`·`FlushOwnedTagsRefresh` 는 모두 `FTickerDelegate` 에 바인딩되는 콜백인데 규칙 4 의 `Handle` 접두가 없다. 같은 파일들의 이벤트 콜백(`HandleGameplayEffectApplied`, `HandleStackCountChanged` 등)은 규칙을 지키고 있어, 모듈 안에서 두 규약이 갈린다.
- **제안**: 규칙대로 개명하거나(`HandleCooldownTick` 등), 매 프레임 갱신 함수는 접두를 붙이지 않는다는 예외를 규칙 쪽에 명시한다. 넷이 일관되게 같은 모양이라 의도된 구분일 가능성이 있다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 7. 🟢 로컬 플레이어를 순회하지만 레이아웃·추적 대상은 하나뿐이다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:29`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:178`
- **범주**: 설계/구조
- **문제**: `Initialize` 는 모든 로컬 플레이어에 대해 `HandleLocalPlayerAdded` 를 돌리고 각각 `OnPlayerControllerChanged` 를 구독하는데, 실제 상태는 `PrimaryGameLayout`·`TrackedPlayerController` 단수다. 두 번째 로컬 플레이어가 붙으면 그 PC 신호가 첫 번째 플레이어의 레이아웃을 제거하고 자기 것으로 갈아치운다(`:188-192`). v1 이 싱글 전제라 문제가 드러나지 않지만, 순회 코드가 스플릿스크린을 지지하는 것처럼 읽혀 오해를 만든다.
- **제안**: 지금 구조를 유지한다면 단일 로컬 플레이어 전제를 헤더 주석에 명시한다. 스플릿스크린을 볼 계획이면 레이아웃·추적 상태를 `ULocalPlayer` 키로 묶는다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp` 및 대응 Public 헤더 전부
- **확인한 규칙 항목**: 모듈 참조는 `WxCore` 만(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h` 모두 WxCore 소속, 다른 Wx 플러그인 include 0건), 60개 파일 전부 첫 줄 저작권 문구 일치, `FORCEINLINE`·헤더 인라인 정의 없음(StateTree `GetInstanceDataType()` 2건은 사유 주석이 달린 규칙 6 예외), `Deinitialize`/`BeginDestroy` 오버라이드의 `Super` 호출 누락 없음
- **미검토 / 한계**: WBP 위젯 계층·MVVM 바인딩 그래프(범위 밖), 런타임 동작 미확인(정적 리뷰만). 발견 2의 미사용 판정은 `Content` 하위 일부 폴더(`WorldObject`·`Megascans`·`MetaHumans` 등 대용량 에셋 폴더)를 제외한 grep 이므로, 삭제 전 에디터의 참조 뷰어로 한 번 더 확인하는 편이 안전하다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 60파일 — `/module-review`로 갱신*
