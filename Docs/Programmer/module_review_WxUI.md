# WxUI — 코드 리뷰

> CommonUI 레이어 스택과 MVVM 뷰모델 계층 모두 경계가 뚜렷하고, 구독 해제·재진입·널 경로를 주석까지 붙여 가며 꼼꼼히 다룬 편이다. 이번 리뷰는 `*.Build.cs`/`.uplugin`과 전체 헤더를 훑은 뒤 System·MVVM·Indicator·Subtitle의 cpp를 깊게 봤고, WBP 내부 구조와 콘텐츠 에셋은 범위 밖이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 공유 뷰모델 `UWxViewModel_AbilitySystem` 을 붙잡는 하드 참조가 없어 GC 대상이다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:23`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:71-77`
- **범주**: 버그/정확성
- **문제**: `GetOrCreate` 는 `NewObject<UWxViewModel_AbilitySystem>(InASC)` 로 만들고 아무 데도 저장하지 않는다. UE 의 GC 는 Outer 를 "객체 → Outer" 방향 참조로만 수집하므로, Outer 가 ASC 라는 사실은 이 VM 을 살려 두지 않는다. 자식 VM(`UWxViewModel_Attribute`/`_Ability`)은 부모를 역참조하지 않고(`CachedASC` 는 약참조), 리졸버는 부모가 아니라 자식만 뷰에 돌려준다(`Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp:19-20`, `Private/MVVM/WxViewModel_Attribute.cpp:158-160`). 따라서 어빌리티/어트리뷰트 리졸버 경로에서는 부모 VM 을 붙잡는 참조가 하나도 없다.
  결과: GC 한 번이 돌면 부모가 수거되고, 다음 `GetOrCreate` 가 새 인스턴스를 만든다 → 살아 있는 기존 자식 VM 을 찾지 못해 **중복 자식 VM 이 생기고, 그만큼 ASC 델리게이트 구독과 쿨다운 티커가 중복 누적**된다. `ActiveEffectViewModels` 도 함께 사라졌다가 재구축된다. `FindSharedViewModel` 이 Garbage 를 일부러 걸러내는 것(`Private/MVVM/WxViewModel.cpp:18`)은 이 수명 문제가 이미 관측됐다는 정황이다.
  (네임플레이트 경로는 `UWxViewModel_Character::AbilitySystem` 이 UPROPERTY 로 붙잡아 무사하다 — 문제는 플레이어 HUD 쪽이다.)
- **제안**: ASC 를 Outer 로 쓰되 수명을 명시적으로 고정한다. 가장 작은 수정은 자식 VM 이 부모를 `UPROPERTY` 로 들고 있게 해 뷰 → 자식 → 부모 사슬을 잇는 것이고, 아니면 `UWxViewModel_Subtitle` 처럼 `UMVVMGameSubsystem` 컬렉션에 ASC별 컨텍스트로 등록하거나 ASC 소유 컴포넌트/서브시스템에 UPROPERTY 맵으로 보관한다.
- **확신도**: 중간 (GC 동작에 대한 정적 판단이다. MVVM 뷰가 리졸버 결과의 상위 체인까지 붙잡는 게 확인되면 무효)

### 2. 🟡 쿨다운 중 매 프레임 `CanActivateAbility`/`CheckCost` 재평가 + 활성 GE 전수 스캔
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:340`, `:379`, `:405-406`
- **범주**: 성능/안전
- **문제**: `UpdateCooldownState` 는 코어 티커에 실려 매 프레임 돌면서 (a) `ASC->GetActiveEffects(Query)` 로 `TArray` 를 새로 할당하고 ASC 의 전체 활성 GE 를 스캔하며, (b) 끝에서 `RefreshActivationState()` 를 부른다. `RefreshActivationState` 는 `CanActivateAbility` 와 `CheckCost` 를 각각 호출하는데, 엔진의 `CanActivateAbility` 는 내부에서 이미 `CheckCost` 를 수행하므로 비용 평가가 프레임당 2회다. `CheckCost` → `CanApplyAttributeModifiers` 는 매번 `FGameplayEffectContext` 와 `FGameplayEffectSpec` 을 새로 만들고 모디파이어 크기를 계산한다. HUD 에 어빌리티 슬롯이 N개면 이 비용이 N배로 붙는다.
  덧붙여 `:38` 의 `RegisterGenericGameplayTagEvent` 구독은 ASC 의 **모든** 태그 변화마다 `RefreshActivationState` 를 다시 돌린다. 전투 중 태그 교체가 잦은 프로젝트라 실제 호출량이 크며, 같은 모듈의 `UWxNameplateComponent` 는 필요한 태그만 좁혀 구독해 정반대 패턴을 쓰고 있다(`Private/Component/WxNameplateComponent.cpp:113-117`).
- **제안**: `RefreshActivationState` 에서 `CheckCost` 를 따로 부르지 말고 `CanActivateAbility` 의 `OptionalRelevantTags` 출력으로 비용 실패 여부를 읽는다. 매 프레임 재평가는 값이 실제로 바뀌었을 때만 브로드캐스트되도록 이미 걸러지므로, 호출 자체를 저빈도(예: 0.1초)로 낮추거나 비용 어트리뷰트/태그 변경 시점 갱신만 남긴다. 태그 구독도 어빌리티의 블록/필요 태그로 좁힌다.
- **확신도**: 높음

### 3. 🟡 `BlueprintCallable` 사용처 규칙 위반 (CLAUDE.md 코딩 규칙 5)
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:42`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxTabListWidgetBase.h:66`, `:72`, `:75`, `:78`, `:81`, `:84`, `:87`
- **범주**: 규칙 위반
- **문제**: 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수로만 제한한다(이 프로젝트의 예외는 "위젯 서브클래스의 MVVM 바인딩 수신용 1-arg setter"). `UWxViewModel_Ability::TryActivateAbility()` 는 뷰모델의 무인자 액션이라 어느 쪽에도 해당하지 않고, `UWxTabListWidgetBase` 의 7개(`GetPreregisteredTabInfo`·`SetTabHiddenState`·`RegisterDynamicTab`·`IsFirstTabActive`·`IsLastTabActive`·`IsTabVisible`·`GetVisibleTabCount`)도 1-arg setter 가 아니다. 그중 4개는 `BlueprintCallable, BlueprintPure` 를 함께 달아 `BlueprintCallable` 자체가 잉여이기도 하다.
- **제안**: 뷰모델 액션은 `UWxUILibrary` 같은 파사드로 옮기거나 BP 노출을 걷는다. TabList 의 순수 조회 7개는 `BlueprintPure` 만 남기고, 상태를 바꾸는 `SetTabHiddenState`/`RegisterDynamicTab` 은 노출 필요성을 재검토한다.
- **확신도**: 높음

### 4. 🟡 `UWxViewModel_Effect` — 무한 지속 GE 의 스택 수가 영원히 갱신되지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:34-52`, `:145-179`
- **범주**: 버그/정확성
- **문제**: 티커는 `CachedDuration > 0.f` 일 때만 등록되고, `StackCount` 를 갱신하는 유일한 경로가 그 티커(`:162`)다. Duration 이 없는(Infinite) 스택형 GE 는 `Initialize` 시점의 스택 수로 고정된 채 남는다 — 스택이 쌓이거나 줄어도 UI 는 그대로다. GAS 는 `OnGameplayEffectStackChangeDelegate` 로 스택 변화를 알려 주므로 폴링에 의존할 이유가 없다.
  같은 함수의 부수 문제: `UpdateEffectState` 가 `false` 를 반환하는 세 경로(`:150`, `:159`, `:169`)가 `TickerHandle` 을 비우지 않는다. 형제 클래스인 `UWxViewModel_Ability::UpdateCooldownState` 는 같은 상황에서 반드시 `TickerHandle.Reset()` 하며 그 이유를 주석으로 남겼다(`Private/MVVM/WxViewModel_Ability.cpp:318-320`). Effect VM 은 재등록 경로가 없어 지금은 무해하지만 규약이 어긋나 있다.
- **제안**: `Initialize` 에서 `ASC->OnGameplayEffectStackChangeDelegate(BoundHandle)` 를 구독해 스택을 이벤트로 받고, 티커는 남은 시간 표시 전용으로 남긴다. `UpdateEffectState` 의 조기 반환에도 `TickerHandle.Reset()` 을 넣어 Ability VM 과 규약을 맞춘다.
- **확신도**: 중간 (무한 지속 스택 GE 를 실제로 UI 에 노출하는지에 달렸다)

### 5. 🟢 `UWxNameplateComponent::InitializeViewModels` 의 널 ASC 무방비 역참조와 조용한 실패
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:89-121`
- **범주**: 버그/정확성
- **문제**: `:115` 에서 `InASC->RegisterGameplayTagEvent(...)` 를 검사 없이 부른다. 바로 앞의 `CharacterViewModel->Initialize(InASC, ...)` 는 널이면 조용히 반환하므로, 널이 들어오면 VM 은 비어 있고 여기서 크래시한다. 유일한 호출자(`Source/WxGame/Character/WxEnemyCharacter.cpp:43`)가 기본 서브오브젝트를 넘겨 지금은 안전하지만, public API 로 노출된 이상 보증이 없다.
  또 `GetWidget()`/`GetExtension<UMVVMView>()` 가 없으면 로그 없이 반환하는데, 이 경우 태그 구독이 걸리지 않아 네임플레이트가 생성자 기본값(숨김)에서 영영 나오지 못한다 — 위젯 클래스를 지정하지 않은 저작 실수가 아무 단서 없이 묻힌다.
- **제안**: 함수 진입부에 `InASC` 널 가드를 두고, View/Widget 부재 경로에 `UE_LOG(LogWxUI, Warning, ...)` 를 남긴다.
- **확신도**: 높음

### 6. 🟢 쿨다운 GE 스캔·티커 등록이 세 곳에 중복
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:57-90`, `:285-296`, `:333-360`
- **범주**: 중복/복잡도
- **문제**: `SeedActiveCooldown` 과 `UpdateCooldownState` 가 같은 `FGameplayEffectQuery{EffectDefinition=CachedCooldownClass}` 로 활성 GE 를 훑어 `GetEffectContext().GetAbility()` 로 거르는 루프를 각각 갖고 있고, 티커 등록 블록(`!TickerHandle.IsValid()` 게이트 + `AddTicker`)은 `SeedActiveCooldown`(`:85-90`)과 `HandleGameplayEffectApplied`(`:291-296`) 두 곳에 그대로 복제돼 있다. 쿨다운 판정 규칙을 고칠 때 세 곳을 함께 고쳐야 한다.
- **제안**: "이 어빌리티의 활성 쿨다운 GE 를 훑어 개수·최단 잔여·지속시간을 낸다" 를 private 헬퍼 하나로 모으고, 티커 등록도 `EnsureCooldownTicker()` 한 곳으로 합친다.
- **확신도**: 높음

### 7. 🟢 `GetCost` 라는 이름이 구독 등록·상태 기록이라는 부수효과를 감춘다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:414-478`
- **범주**: 설계/구조
- **문제**: `Get` 접두 함수가 값을 돌려주지 않고 `CostAttribute`/`CostMaxAttribute` 를 기록한 뒤 ASC 어트리뷰트 변경 델리게이트 두 개를 구독한다. 해제는 멀리 떨어진 `Deinitialize`(`:100-107`)에 있어, 이름만 보고는 짝이 되는 해제가 필요한 함수인지 알 수 없다.
- **제안**: `InitializeCostTracking` 처럼 구독 수명을 드러내는 이름으로 바꾼다.
- **확신도**: 높음

### 8. 🟢 사망·대화 화면 push 가 게임플레이 도중 동기 로드를 탄다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:75-83`, 사용처 `:245`, `:285`, `:298`
- **범주**: 성능/안전
- **문제**: `PushSoftContentToLayer` 는 `LoadSynchronous()` 로 위젯 클래스를 끌어온다. HUD(빙의 직후)는 몰라도 사망 태그·대화 태그가 붙는 순간은 전투/연출 한복판이라, 아직 스트리밍되지 않은 화면 클래스면 그 프레임에 히치가 그대로 보인다. 같은 모듈에 비동기 push 경로(`Public/Widget/WxAsyncAction_PushWidgetToLayer.h`)가 이미 있다.
- **제안**: 개발자 설정의 화면 클래스들을 레이아웃 생성 시점에 한 번 미리 스트리밍해 두거나, 사망/대화 push 를 비동기 경로로 돌린다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 클래스가 작아 실측 히치가 없을 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorManagerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Public/**` 전 헤더, `Plugins/WxUI/Source/WxUI/Private/Widget/*.cpp`(TabList·Button·Popup·HUDLayout·AsyncAction·ActionWidget), `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicatorDescriptor.cpp`, 소비 측 확인용 `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`·`Source/WxGame/Character/WxEnemyCharacter.cpp`
- **미검토 / 한계**: (1) 발견 1의 GC 판단은 정적 분석이며 실제 GC 로그로 확인하지 않았다. (2) `Plugins/WxUI/Content/` 의 WBP·리졸버 저작 상태를 보지 않아, 각 뷰모델의 실제 바인딩 빈도(발견 2의 체감 비용)와 인디케이터 슬롯 개수 전제를 검증하지 못했다. (3) `UWxIndicatorManagerComponent` 의 화면 클램프 기하(평면 교차)는 로직을 읽기만 하고 수치 검증은 하지 않았다. (4) 스플릿스크린(로컬 플레이어 2인 이상)에서 `UWxUIManagerSubsystem` 이 레이아웃·추적 PC 를 단일 필드로 두는 점은 v1 단일 플레이어 전제로 보고 발견에 넣지 않았다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 61파일 — `/module-review`로 갱신*
