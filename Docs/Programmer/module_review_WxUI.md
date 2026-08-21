# WxUI — 코드 리뷰

> CommonUI 레이어 스택과 MVVM 뷰모델 계층 모두 경계가 뚜렷하고, 구독 해제·재진입·널 경로·GC 수명을 주석까지 붙여 가며 다뤄 둔 편이다(직전 리뷰가 지적한 네임플레이트 널 가드와 쿨다운 중복 스캔은 이미 정리됐다). 이번 리뷰는 `WxUI.Build.cs`/`.uplugin`과 전체 헤더를 훑은 뒤 System·MVVM·Indicator·Subtitle·Widget의 cpp를 전부 읽었고, 판정이 갈리는 GAS·GC·Streamable 동작은 UE 5.8 엔진 소스로 대조했다. WBP 내부 구조와 콘텐츠 에셋은 범위 밖이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 `MaxRecharges` 의 출처와 `CurrentCharges` 계산이 양립 불가능한 쿨다운 GE 설정을 각각 요구한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:21-29`, `:69-107`, `:332`, `:357`
- **범주**: 버그/정확성
- **문제**: `MaxRecharges` 는 쿨다운 GE 의 `StackLimitCount` 에서 읽는다(`:24`). 그런데 엔진에서 `StackLimitCount` 는 `StackingType != None` 일 때만 편집·의미가 있다(`GameplayEffect.h` 의 `EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"`). 반면 `QueryActiveCooldowns` 는 **활성 GE 엔트리 개수**를 세어 소모된 충전 수로 삼는데(`:98-106`, 주석 "활성 쿨다운 GE 1개 = 회복 대기 중인 충전 1개"), 엔진은 `StackingType != None` 이면 같은 GE 의 재적용을 하나의 `FActiveGameplayEffect` 로 합친다(`FActiveGameplayEffectsContainer::FindStackableActiveGameplayEffect`).
  결과: `MaxRecharges > 1` 을 만들 수 있는 유일한 저작(스택형 쿨다운 GE)에서 `QueryActiveCooldowns` 는 절대 1을 넘지 못하고, `CurrentCharges = MaxRecharges - ConsumedCharges`(`:357`)가 항상 `MaxRecharges - 1` 로 굳는다. 3충전 어빌리티를 두 번 써도 UI 는 2충전 남았다고 표시한다. 반대로 `StackingType == None` 으로 두면 개수 세기는 맞지만 `StackLimitCount` 가 0이라 `MaxRecharges` 가 1로 접혀 다중 충전 표시 자체가 꺼진다.
- **제안**: 두 축을 한쪽으로 맞춘다. 스택형을 전제로 간다면 엔트리 개수 대신 `ActiveGE.Spec.GetStackCount()` 를 소모 충전 수로 쓰고, 비스택형을 전제로 간다면 `MaxRecharges` 출처를 `StackLimitCount` 가 아니라 어빌리티 쪽 저작 필드(예: `UWxAbilityComponent`)로 옮긴다.
- **확신도**: 중간 (엔진 동작은 확인했으나, 다중 충전 어빌리티가 실제 콘텐츠에 존재하는지는 확인하지 못했다 — 아직 안 쓰는 기능이면 잠재 결함)

### 2. 🟡 `UWxViewModel_Effect` — 무한 지속 GE 의 스택 수가 초기값에 고정된다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:33-52`, `:145-179`
- **범주**: 버그/정확성
- **문제**: 티커는 `CachedDuration > 0.f` 일 때만 등록되고(`:34`, `:49-51`), `StackCount` 를 갱신하는 유일한 경로가 그 티커다(`:162`). Infinite GE 는 `GetDuration()` 이 `INFINITE_DURATION(-1)` 이므로 티커가 아예 붙지 않아 `Initialize` 시점 스택 수로 얼어붙는다. 이벤트로 보완되지도 않는다 — 엔진은 기존 스택에 얹히는 적용에서 `OnActiveGameplayEffectAddedDelegateToSelf` 를 브로드캐스트하지 않고(`FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec` 의 `ExistingStackableGE` 분기는 `OnStackCountChange` 만 호출) `UWxViewModel_AbilitySystem::HandleActiveEffectAdded` 는 애초에 호출되지 않으며, WxUI 어디에서도 `OnGameplayEffectStackChangeDelegate` 를 구독하지 않는다.
  같은 함수의 부수 문제: `UpdateEffectState` 가 `false` 를 돌려주는 세 경로(`:150`, `:159`, `:169`)가 `TickerHandle` 을 비우지 않는다. 형제 클래스 `UWxViewModel_Ability::UpdateCooldownState` 는 같은 자리를 반드시 비우고 그 이유를 주석으로 남겼다(`Private/MVVM/WxViewModel_Ability.cpp:323-326`). Effect VM 은 재등록 경로가 없어 지금은 무해하지만, 나중에 재등록을 붙이면 곧바로 굳는다.
- **제안**: `Initialize` 에서 `ASC->OnGameplayEffectStackChangeDelegate(BoundHandle)` 를 구독해 스택을 이벤트로 받고, 티커는 남은 시간 표시 전용으로 남긴다. 조기 반환 세 곳에도 `TickerHandle.Reset()` 을 넣어 Ability VM 과 규약을 맞춘다.
- **확신도**: 중간 (엔진 동작은 확인했다. 무한 지속 + 스택 + `UWxEffectComponent_UIData` 조합을 실제로 쓰는지에 영향 범위가 달렸다)

### 3. 🟡 `BlueprintCallable` 사용처 규칙 위반 (CLAUDE.md 코딩 규칙 5)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`, `:72`, `:75`, `:78`, `:81`, `:84`, `:87`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:42`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxButtonBase.h:20`
- **범주**: 규칙 위반
- **문제**: 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리로 제한한다(프로젝트 예외는 "위젯 서브클래스의 MVVM 바인딩 수신용 1-arg setter"). 모듈에서 허용 범위인 것은 `UWxUILibrary`(`Public/WxUILibrary.h:40`, `:43`, `:46`, `:49`)와 `UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer`(`Public/Widget/WxAsyncAction_PushWidgetToLayer.h:27`) 뿐이다.
  나머지 9개는 해당 없음: `UWxTabListWidgetBase` 의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)는 setter 가 아니며 그중 4개는 `BlueprintPure` 를 함께 달아 `BlueprintCallable` 자체가 잉여다. `UWxViewModel_Ability::TryActivateAbility()` 는 위젯이 아니라 뷰모델의 무인자 액션이다. `UWxButtonBase::SetButtonText` 는 위젯 1-arg setter 이긴 하나 MVVM 바인딩 수신용이 아니라 코드/BP 직접 호출용이다(`Private/Widget/WxTabButtonBase.cpp:26`).
- **제안**: TabList 의 순수 조회 7개는 `BlueprintPure` 만 남긴다. 뷰모델 액션은 `UWxUILibrary` 파사드로 옮기거나 노출을 걷는다. `SetButtonText` 는 예외로 인정할지 판정해 문서화하거나 노출을 걷는다.
- **확신도**: 높음 (`UWxTabListWidgetBase`/`UWxButtonBase` 는 Lyra 이식본이라 의도적 예외일 여지는 있다)

### 4. 🟢 사망·대화·HUD 화면 push 가 게임플레이 도중 동기 로드를 탄다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:75-83`, 사용처 `:104`, `:244`, `:284`, `:297`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:94`
- **범주**: 성능/안전
- **문제**: `PushSoftContentToLayer` 는 `LoadSynchronous()` 로 위젯 클래스를 끌어온다. 빙의 직후 HUD 는 몰라도, 사망 태그·대화 태그가 붙는 순간과 확인 팝업(`:104`)·메뉴 토글(`WxHUDLayout.cpp:94`)은 전투/연출 한복판이라, 아직 스트리밍되지 않은 화면 클래스면 그 프레임 히치가 그대로 보인다. 같은 모듈에 비동기 push 경로(`Public/Widget/WxAsyncAction_PushWidgetToLayer.h`)가 이미 있다.
- **제안**: 개발자 설정의 화면 클래스들을 레이아웃 생성 시점에 한 번 미리 스트리밍해 두거나, 사망/대화 push 를 비동기 경로로 돌린다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 위젯이 작아 실측 히치가 없을 수 있다)

### 5. 🟢 `GetCost` 라는 이름이 구독 등록·상태 기록이라는 부수효과를 감춘다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:402-466`, 짝이 되는 해제는 `:116-123`
- **범주**: 설계/구조
- **문제**: `Get` 접두 함수가 값을 돌려주지 않고 `CostAttribute`/`CostMaxAttribute` 를 기록한 뒤 ASC 어트리뷰트 변경 델리게이트 두 개를 구독한다(`:458-465`). 해제는 멀리 떨어진 `Deinitialize` 에 있어, 호출부만 봐서는 짝이 되는 해제가 필요한 함수인지 알 수 없다.
- **제안**: `InitializeCostTracking` 처럼 구독 수명을 드러내는 이름으로 바꾼다.
- **확신도**: 높음

### 6. 🟢 티커 델리게이트 콜백에 `Handle` prefix 누락 (CLAUDE.md 코딩 규칙 4)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:64-66`(선언 `Public/MVVM/WxViewModel_Ability.h:133`), `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:49-51`(선언 `Public/MVVM/WxViewModel_Effect.h:74`)
- **범주**: 규칙 위반
- **문제**: `UpdateCooldownState`/`UpdateEffectState` 는 `FTickerDelegate::CreateUObject` 로 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 가 없다. 모듈의 다른 델리게이트 콜백(`HandleImageLoaded`·`HandleTagChanged`·`HandleOwnedTagsChanged` 등)은 모두 규칙을 지키고 있어 이 둘만 어긋난다.
- **제안**: `HandleCooldownTick`/`HandleEffectTick` 등으로 개명한다.
- **확신도**: 중간 (틱 콜백을 규칙 4의 "Delegate 콜백"으로 볼지 해석 여지가 있다)

### 7. 🟢 불필요한 람다 (CLAUDE.md 코딩 규칙 3)
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp:43-46`
- **범주**: 규칙 위반
- **문제**: `PreregisteredTabInfoArray.FindByPredicate([&](FWxTabDescriptor& TabInfo){ return TabInfo.TabId == TabNameId; })` 는 단순 선형 탐색이다. 같은 파일의 `SetTabHiddenState`(`:62-72`)가 같은 배열에 대한 같은 탐색을 명시 루프로 하고 있어, 한 파일 안에서 두 방식이 공존한다.
- **제안**: `SetTabHiddenState` 와 같은 명시 루프로 통일한다.
- **확신도**: 중간 (이식 코드의 원형이라 그대로 둔 것일 수 있다)

### 8. 🟢 `UWxActionWidget::GetIcon` 이 디자인타임에 프로젝트 전체 IMC 를 강제 로드한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxActionWidget.cpp:29-36`
- **범주**: 성능/안전
- **문제**: `GetAssetsByClass` 로 모든 `UInputMappingContext` 에셋을 열거한 뒤 매칭 여부와 무관하게 `MappingContextAsset.GetAsset()` 으로 하나씩 강제 로드한다. `GetIcon()` 은 위젯 갱신마다 불리므로, IMC 가 늘수록 UMG 에디터에서 액션 위젯을 건드릴 때마다 전체 재스캔·재순회가 붙는다(로드 자체는 1회지만 순회는 매번이다). 런타임 영향은 없다.
- **제안**: 액션→키 해석 결과를 CDO/정적 맵에 캐시하거나, 프로젝트가 실제로 쓰는 IMC 목록을 설정으로 좁힌다.
- **확신도**: 중간 (에디터 전용이라 체감 비용은 IMC 개수에 달렸다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxTabListWidgetBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/README.md`, `Plugins/WxUI/Source/WxUI/Public/**` 전 헤더, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`·`WxButtonBase.cpp`·`WxTabButtonBase.cpp`·`WxActionWidget.cpp`·`WxGamePopup.cpp`·`WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`·`WxViewModel_Interaction.cpp`·`WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxEffectComponent_UIData.cpp`, 소비 측 확인용 `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`·`Source/WxGame/Character/WxEnemyCharacter.cpp`
- **미검토 / 한계**:
  1. **직전 리뷰의 🔴(공유 `UWxViewModel_AbilitySystem` 이 GC 대상)은 재검증 결과 오탐이다.** UE 의 GC 는 객체마다 `Class`·**`Outer`**·ExternalPackage 를 무조건 참조로 방출하므로(`Engine/Source/Runtime/CoreUObject/Public/UObject/FastReferenceCollector.h` 의 `HandleImmutableReference(Outer, EMemberlessId::Outer, ...)`), 리졸버가 돌려준 자식 VM 을 MVVM 뷰가 붙잡으면 자식 → Outer(부모 VM) → Outer(ASC) 사슬로 부모가 살아남는다. `Public/MVVM/WxViewModel_AbilitySystem.h:33` 의 주석이 맞다. 같은 항목을 다시 열지 않도록 여기 남긴다.
  2. 모듈 경계는 이상 없다 — `WxUI.Build.cs` 의 Wx 의존은 `WxCore` 뿐이고, 소스 전체 `#include` 에도 다른 Wx 플러그인 헤더가 없다(WxCore 의 `WxGameplayTags.h` 만 참조). 첫 줄 저작권 표기 누락 0건, `FORCEINLINE`/인라인 정의 0건(StateTree 태스크의 `GetInstanceDataType()` 은 코드에 예외 근거가 명시돼 있다).
  3. `Plugins/WxUI/Content/` 의 WBP·리졸버 저작 상태를 보지 않았다. 발견 1·2의 실제 영향 범위(다중 충전 어빌리티·무한 지속 스택 GE 가 콘텐츠에 존재하는지)와 인디케이터 슬롯 개수 전제를 검증하지 못했다.
  4. `UWxIndicatorManagerComponent` 의 화면 클램프 기하(평면 교차, `Private/Indicator/WxIndicatorManagerComponent.cpp:53-103`)는 로직을 읽기만 하고 수치 검증은 하지 않았다.
  5. 스플릿스크린(로컬 플레이어 2인 이상)에서 `UWxUIManagerSubsystem` 이 레이아웃·추적 PC·HUD 를 단일 필드로 두고 `OnLocalPlayerRemovedEvent` 를 구독하지 않는 점, `RefreshGamePause` 가 `NM_Standalone` 에서만 도는 점은 v1 싱글 전제(코드 주석에도 명시)로 보고 발견에 넣지 않았다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 61파일 — `/module-review`로 갱신*
