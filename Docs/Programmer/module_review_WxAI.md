# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹은 설계가 잘 지켜져 있고 프로젝트 코딩·모듈 규칙 위반은 0건이다. 알려진 함정(GAS Spec 배열 재할당·latent 동기 종료, Composite 노드 메모리 레이아웃, 델리게이트 바인드/언바인드 대칭)도 이미 방어돼 있다. 남은 문제는 두 갈래에 몰려 있다 — (a) 파생 상태(`State.InCombat`·회전 모드·`MaxWalkSpeed`)를 발행해 놓고 무효화 경로가 없는 패턴, (b) 커스텀 Composite/Decorator 가 엔진의 옵저버 등록 규약을 우회해 FlowAbortMode 안내가 실제로 동작하지 않는 지점. 커버리지: 소스 29파일 전부를 읽었고 `WxAIPerceptionComponent`·`WxBTComposite_RandomChoice`·`WxBTTask_ActivateAbility`·MoveTo 파생 태스크·리시 한 쌍은 cpp 로직까지 정독했으며, UE 5.8 `AIModule` 원본과 소비자(`AWxEnemyController`·`AWxCharacterBase`)에 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 SetTargetActor 의 조기 반환이 회전 모드를 strafe 에 고착시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:215`, `:245-250`
- **범주**: 버그/정확성
- **문제**: `SetTargetActor` 는 "BB 의 현재 TargetActor == NewTarget" 이면 즉시 반환하고, 뒤의 파생 상태 원복(`AIC->ClearFocus`, `bUseControllerDesiredRotation = false`, `bOrientRotationToMovement = true`)에 도달하지 못한다. 그런데 Blackboard 의 Object 키는 엔진이 `FWeakObjectPtr` 로 저장하므로(UE 5.8 `BlackboardKeyType_Object.cpp:13`, `:29`) 타겟 액터가 파괴되면 BB 값이 컴포넌트 모르게 스스로 nullptr 이 된다. 이 상태에서 뒤늦게 `SetTargetActor(nullptr)` 이 불리면(억제 진입 `:160`, 사망 정리 `:203`) "이미 nullptr" 이라 조기 반환하고, 폰은 strafe 회전 모드에 갇힌다. 포커스 대상도 이미 소실돼 `AAIController::UpdateControlRotation` 이 갱신을 멈추므로 이후 정찰·복귀 이동에서 진행 방향을 보지 않고 미끄러지듯 이동하며, 새 타겟을 잡기 전까지 자력 복구되지 않는다.
- **제안**: "마지막으로 적용한 타겟" 을 BB 가 아니라 컴포넌트 자체 필드(`TWeakObjectPtr<AActor>`)로 들고 그것과 비교한다. BB 쓰기와 파생 상태 발행의 판단 기준을 분리하면 BB 의 자체 무효화와 무관해진다.
- **확신도**: 높음 (BB 키의 약참조 저장은 엔진 원본으로 확인)

### 2. 🟡 타겟이 죽거나 파괴돼도 State.InCombat 이 해제되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:31`, `:88`, `:118`
- **범주**: 설계/구조
- **문제**: `UpdateRecognition()` 의 유일한 호출부가 `HandleTargetPerceptionUpdated` 이고(`:88`), 이 콜백은 `OnTargetPerceptionUpdated` 에만 바인드돼 있다(`:31`). 즉 인식 상태는 "새 자극이 올 때" 만 재판정된다. 두 경로 모두 재판정을 못 받는다 — (a) 사망한 캐릭터는 파괴되지 않고 시야에 남아 새 자극을 만들지 않는다. (b) 타겟이 실제로 파괴되면 엔진 헤더(`Perception/AIPerceptionComponent.h:417`)가 명시하듯 `OnTargetPerceptionUpdated` 는 아예 호출되지 않고, BB 값만 조용히 null 이 된다. 결과적으로 `State.InCombat` 이 폰 ASC 에 켜진 채 남는다. 이 태그를 쓰거나 지우는 코드는 이 컴포넌트가 유일하므로(저장소 전체 grep 확인) 아무도 정리해 주지 않고, MinimalReplication 으로 클라에 복제돼 네임플레이트(`Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:27`)가 계속 전투 표시를 한다. 자기 자신의 사망은 `BindOwnerDeath`/`HandleDeathTagChanged`(`:165`, `:194`)로 정확히 이 문제를 막고 있는데 타겟 쪽 대칭 처리만 비어 있다. 리시 복귀 억제도 구제가 안 된다 — 홈 반경 안에서 타겟이 사라지면 `UWxBTTask_ReturnHome` 자체가 실행되지 않는다.
- **제안**: `UpdateRecognition()` 을 자극 이벤트 외에도 펌프한다. `SetTargetActor` 에서 새 타겟 ASC 의 `State.Dead` 를 구독/해제하는 방식(자기 폰용 `BindOwnerDeath` 와 같은 패턴)이 사망·파괴 양쪽을 덮고, `OnTargetPerceptionForgotten`/`OnTargetPerceptionInfoUpdated` 추가 구독은 파괴만 덮는다. 어느 쪽이든 판정 자체는 기존 `UpdateRecognition` 단일 지점에 그대로 위임한다.
- **확신도**: 높음 (메커니즘 확정) / 중간 (BT 에셋 측에 타겟 생존 게이트가 있으면 체감이 줄 수 있음)

### 3. 🟡 RandomChoice 의 사전 필터가 조건 Decorator 의 FlowAbortMode 를 무력화한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:68`
- **범주**: 버그/정확성
- **문제**: 엔진 `UBTCompositeNode::FindChildToExecute` 는 조건이 막은 자식마다 `NotifyDecoratorsOnFailedActivation` 을 부르고, 그 안에서 `LowerPriority`/`Both` FlowAbortMode 를 가진 Decorator 를 aux 옵저버로 등록한다(UE 5.8 `BTCompositeNode.cpp:297-315`, 특히 `:307-311` 의 `AddUniqueUpdate(... EBTNodeUpdateMode::Add)`). `GetNextChildHandler` 의 사전 필터는 조건 false 인 자식을 `continue` 로 건너뛸 뿐이라 이 등록이 전혀 일어나지 않는다. 결과: RandomChoice 아래 자식에 조건 데코를 달고 FlowAbortMode 를 LowerPriority 로 지정해도, 조건이 참으로 뒤집히는 순간 진행 중인 하위 패턴을 끊고 들어오지 못한다. 평범한 Selector 였다면 동작했을 "조건 충족 즉시 선점" 구성이 이 컴포지트 아래에서만 조용히 죽는다. 7번과 짝을 이루며, 둘 다 고쳐야 그 구성이 성립한다.
- **제안**: 필터에서 제외한 인덱스마다 `NotifyDecoratorsOnFailedActivation(SearchData, Index, LastResult)`(`UBTCompositeNode` 의 protected 멤버라 파생 클래스에서 호출 가능)를 불러준다. 그럴 의사가 없다면 헤더에 "RandomChoice 자식의 조건 데코는 FlowAbortMode 가 동작하지 않는다" 를 규약으로 못박는다.
- **확신도**: 중간 (동작은 엔진 원본으로 확정. 현재 BT 에셋이 이 구성을 쓰는지는 미확인)

### 4. 🟡 avoid-repeat 가 "지금 유효한 유일한 자식" 까지 제외해 컴포지트를 실패시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:51`, `:61-64`, `:91-94`
- **범주**: 버그/정확성
- **문제**: 후보 수집이 "직전 선택 자식 제외" → "조건 Decorator 통과 여부" 순으로 돌아, 조건을 통과하는 자식이 직전 선택 자식 하나뿐이면 `Candidates` 가 비어 `ReturnToParent`(실패)가 나간다. 회피를 무시하는 가드는 `ChildrenNum > 1`(`:51`)뿐이라 *실제 유효 후보 수* 가 아니라 *물리적 자식 수* 만 본다. 헤더 주석(`Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:27-28`)의 "자식이 1개뿐이면 회피는 무시되고 항상 그 자식이 선택된다" 는 의도와 어긋난다. 실패 시나리오: 공격 패턴 3개 중 2개가 `UWxBTDecorator_AttributeRatio`(HP 조건)로 막힌 페이즈에서 남은 1개를 한 번 쓰고 나면, 다음 진입마다 컴포지트가 실패해 전투 브랜치가 하위 우선순위(정찰·배회)로 떨어진다 — 보스가 전투 중 걸어 나가는 형태로 보인다.
- **제안**: 회피 필터 적용 후 `Candidates` 가 비면 회피를 풀고 조건 통과 후보만으로 한 번 더 수집해 재추첨한다(회피를 "가능하면" 규칙으로). 엄격 avoid 가 의도라면 이 실패 거동을 헤더 시멘틱 주석에 명시한다.
- **확신도**: 중간 (상위 Selector 폴백에 의존하는 의도된 설계일 수 있음)

### 5. 🟡 MaxWalkSpeed 저장·복원이 SPD 어트리뷰트의 소유권과 충돌한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:53-61`, `:79-89` / `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:56-64`, `:102-114`
- **범주**: 설계/구조
- **문제**: 두 태스크는 진입 시 `MaxWalkSpeed` 를 캐시해 배율을 곱하고 종료 시 캐시한 **절대값** 으로 되돌린다. 그러나 같은 필드를 `AWxCharacterBase::HandleSPDAttributeChanged`(`Source/WxGame/Character/WxCharacterBase.cpp:238`)가 `MaxWalkSpeed = BaseWalkSpeed * SPD` 로 어트리뷰트 변경마다 절대값 재계산한다. 소유자가 둘이라 (a) 정찰 중 SPD 가 바뀌면 정찰 감속 배율이 통째로 사라지고, (b) 정찰 중 걸린 버프/디버프가 태스크 종료 시 "정찰 진입 시점" 의 낡은 절대값으로 덮어써져 다음 SPD 이벤트까지 무효화된다. 예: Base 400 / SPD 1 → 정찰 진입(캐시 400, 실제 200) → 가속 버프 SPD 2(실제 800) → 정찰 종료 시 400 으로 복원되어 버프가 남은 시간 동안 사라진다. 덧붙여 `MoveSpeedMultiplier` 의 `ClampMin = 0.0`(`Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h:32`)은 0 을 허용해, 0 을 넣으면 Patrol 의 MoveTo 가 영원히 도착하지 못한다.
- **제안**: 감속을 CMC 필드 직접 쓰기가 아니라 SPD 를 낮추는 GE(진입 시 부여·종료 시 제거)로 적용해 소유권을 한쪽으로 모은다. 최소 대응은 절대값 대신 배율을 되돌리는 것이며, 이때 `ClampMin` 을 0 보다 큰 값으로 올려야 한다. 캐시·복원 블록이 두 파일에 거의 문자 그대로 중복돼 있으므로 어느 방향이든 두 곳을 함께 고쳐야 한다.
- **확신도**: 높음(메커니즘) / 중간(정찰·배회 중 SPD 변동 빈도에 따라 체감이 갈린다)

### 6. 🟡 Sight/Hearing 센스가 외부 ApplySenseSettings 호출에만 의존해 등록된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:38-41`, `:59`, `:65`
- **범주**: 설계/구조
- **문제**: 엔진의 `SensesConfig` 배열은 오직 `ConfigureSense()` 로만 채워지고(UE 5.8 `AIPerceptionComponent.cpp:122-157`; 베이스 `PostInitProperties` 는 DominantSense 만 처리한다), `OnRegister` 는 그 배열에 든 센스만 퍼셉션 시스템에 리스너로 올린다. 이 컴포넌트의 `PostInitProperties` 는 `DamageConfig` 만 등록하고, Sight/Hearing 은 `ApplySenseSettings` 안의 `ConfigureSense` 에서야 등록된다. 그런데 그 유일한 호출부는 `AWxEnemyController::OnPossess` 의 `Cast<AWxEnemyCharacter>` 성공 분기 안이다(`Source/WxGame/Controller/WxEnemyController.cpp:30-32`). 다른 Pawn 타입에 빙의하거나 이 컴포넌트를 BP 에서 단독으로 붙이면(`meta=(BlueprintSpawnableComponent)` 로 노출돼 있다) 경고 한 줄 없이 "피해만 감지하는" AI 가 만들어진다. `WxBlackboardKeys` 가 키 오용을 진단 로그로 드러내는 것과 대비되는 비대칭이다.
- **제안**: `PostInitProperties` 에서 Damage 와 함께 Sight/Hearing 도 기본값으로 `ConfigureSense` 해 두고, `ApplySenseSettings` 는 값 갱신 + `RequestStimuliListenerUpdate` 만 하게 한다(엔진 `ConfigureSense` 는 동일 클래스 재등록을 갱신으로 처리하므로 중복 등록되지 않는다).
- **확신도**: 높음(등록 경로는 엔진 원본으로 확인) / 현 사용처에선 항상 호출되므로 잠재 결함

### 7. 🟡 AttributeRatio 는 FlowAbortMode 를 켜도 실시간 재평가가 일어나지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:16`
- **범주**: 설계/구조
- **문제**: 헤더가 "실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다" 고 안내하지만, 이 Decorator 는 `CalculateRawConditionValue` 만 구현하고 `OnBecomeRelevant`/`TickNode`/어트리뷰트 변경 구독이 전혀 없다(cpp 전체가 생성자·`GetStaticDescription`·조건 계산 3개뿐). FlowAbortMode 는 "abort 를 허용하는 범위" 만 정할 뿐 재평가를 촉발하지 않으므로, HP 가 임계값을 넘어도 다른 원인으로 BT 재탐색이 일어나기 전까지 조건은 갱신되지 않는다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 는 정확히 이 한계 때문에 `TickNode` 폴링 + `RequestExecution` 을 직접 구현했다(`Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:51-63`). 디자이너가 헤더 안내대로 FlowAbortMode 만 켜고 "HP 50% 가 되면 즉시 발악기" 를 기대하면 조용히 어긋난다.
- **제안**: BeyondLeash 처럼 관찰자 경로를 구현한다 — `OnBecomeRelevant` 에서 대상 ASC 의 두 어트리뷰트 변경 델리게이트를 구독해 비율 판정이 뒤집힐 때만 `RequestExecution` 을 호출하고 `OnCeaseRelevant` 에서 해제한다(틱 폴링보다 저렴하다). 구현할 계획이 없다면 헤더 문구를 "재탐색 시점에만 재평가된다" 로 정정한다.
- **확신도**: 중간

### 8. 🟢 사망한 폰이 잔여 자극으로 TargetActor·포커스·회전 모드를 다시 잡는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:82-85` vs `:106-115`
- **범주**: 버그/정확성
- **문제**: `HandleTargetPerceptionUpdated` 는 사망 여부와 무관하게 `SetTargetActor(Actor)` 를 먼저 실행하고, 그 뒤 `UpdateRecognition` 이 `State.Dead` 를 보고 조기 반환한다(`:110-114`). 즉 사망 후 도착한 자극은 인식 태그는 못 켜지만 BB TargetActor 를 다시 채우고 `AIC->SetFocus` + `bUseControllerDesiredRotation = true` 를 시체에 다시 건다. `:106` 의 "잔여 감지 자극이 인식을 재부여하지 않도록 방어한다" 는 주석은 인식 태그만 방어하고 타겟·회전은 방어하지 못한다.
- **제안**: 억제 검사(`:75-78`) 옆에 사망 검사를 함께 두어, 죽은 폰이면 `SetTargetActor` 이전에 조기 반환한다.
- **확신도**: 중간 (BT 가 이미 멈춘 뒤라 실질 영향이 표시상에 그칠 수 있음)

### 9. 🟢 ReturnHome 의 bCreateNodeInstance 와 주석이 실체와 어긋난다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:20-21`, `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h:14`
- **범주**: 중복/복잡도
- **문제**: "이동 속도 캐시를 폰별로 보관하기 위해 노드를 인스턴싱한다" 는 주석과 함께 `bCreateNodeInstance = true` 를 켜지만, `UWxBTTask_ReturnHome` 에는 멤버 변수가 하나도 없다(헤더 `:22-27` 전부 함수). 과거 속도 캐시가 제거되면서 주석과 플래그만 남은 것으로 보이며, 지금은 BT 컴포넌트마다 쓸모없는 노드 인스턴스 UObject + `FBTInstancedNodeMemory` 특수 메모리를 하나 더 만들 뿐이다. 헤더 주석의 "현재 타겟/**마지막 인지 위치** 를 비우고" 도 실제 `SetTargetingSuppressed` 동작(`WxAIPerceptionComponent.cpp:147-163`)에 없는 내용이다. 이 모듈은 주석을 사실상 규약 문서로 쓰고 있어 드리프트의 해악이 크다.
- **제안**: 플래그와 주석을 함께 제거하고(태스크는 무상태이므로 인스턴싱 없이 동작한다), 헤더 문구에서 "마지막 인지 위치" 를 뺀다.
- **확신도**: 높음

### 10. 🟢 인식 해제 주석이 이미 제거된 BGMSourceComponent 를 근거로 든다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:107`
- **범주**: 중복/복잡도
- **문제**: "`SetRecognized(false)` 가 곧 `State.InCombat` 제거이며, 이 태그를 감시하는 BGMSourceComponent 가 시체 위에서 계속 재생되는 것을 막는다" 고 적혀 있으나, 저장소에서 `BGMSource` 를 언급하는 코드는 이 주석 한 줄뿐이다(WxSound BGM 플러그인은 2026-07-29 제거됨, 나머지 히트는 전부 worklog 문서). 2번의 심각도를 판단할 때 이 주석 때문에 소비자를 잘못 가정하게 된다 — 현재 `State.InCombat` 의 실소비자는 네임플레이트(WxUI)다.
- **제안**: 주석에서 BGM 근거를 지우고 현재 소비자로 갱신한다.
- **확신도**: 높음

### 11. 🟢 Wander 가 매 실행마다 방향 후보 배열을 힙 할당한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:29`
- **범주**: 성능/안전
- **문제**: 최대 8개로 고정된 후보를 기본 할당자 `TArray<int32>` 에 담아 배회 진입마다 힙 할당이 발생한다. 같은 모듈의 `UWxBTComposite_RandomChoice` 는 동일 상황에서 `TInlineAllocator<8>` 을 쓴다(`Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:53-54`). 다수의 배회 AI 가 짧은 Duration(기본 1s)으로 반복 진입하면 누적된다.
- **제안**: `TArray<int32, TInlineAllocator<8>>` 로 맞춘다.
- **확신도**: 높음

### 12. 🟢 ActivatableAbilities 순회 중 TryActivateAbility 를 호출하면서 스코프 락을 걸지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:36`, `:48`
- **범주**: 성능/안전
- **문제**: `ASC->GetActivatableAbilities()` 를 range-for 로 돌면서 루프 안에서 `TryActivateAbility` 를 호출한다. 성공 시엔 즉시 `break` 하고 이후 `IterSpec` 을 건드리지 않아 안전하지만, **실패 시엔 계속 순회한다**. `:48` 의 주석은 "실패는 ActivatableAbilities 를 바꾸지 않는다" 고 단정하는데 이는 GAS 가 보장하는 계약이 아니다 — 실패 경로에서도 `NotifyAbilityFailed` 브로드캐스트로 게임 코드가 실행될 수 있고, 그 안에서 `GiveAbility`/`ClearAbility` 가 일어나면 락이 없는 동안 배열이 재할당·`RemoveAtSwap` 되어 순회 중인 참조가 무효화된다.
- **제안**: 루프를 `FScopedAbilityListLock Lock(*ASC);` 으로 감싸 순회 중 배열 변경을 지연시킨다. 한 줄 추가로 가정을 보장으로 바꿀 수 있다.
- **확신도**: 낮음 (현재 어빌리티 구성에선 실제로 발생하지 않을 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp` (+ 대응 헤더)
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/` 헤더 15개 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`. 대조용으로 `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:159`.
- **확인 결과 문제 없던 항목**: `UWxBTTask_ActivateAbility` 의 latent 수명주기(활성화 후 핸들 재조회, 동기 종료 시 `InProgress` 영구 정지 방지, Abort 경로에서 델리게이트 선해제 후 취소 — `FinishLatentTask` 누락·중복 없음). `UWxBTComposite_RandomChoice` 의 노드 메모리 레이아웃(`FBTCompositeMemory` 상속 + `GetInstanceMemorySize` 일치로 베이스 영역 침범 없음)과 룰렛 부동소수 폴백, 그리고 `UBTComposite_Selector` 상속이 `GetNextChildHandler` 오버라이드를 가리지 않음(엔진 `BTComposite_Selector.cpp` 는 `OnNextChild` 를 바인드하지 않는다). `UWxBTDecorator_BeyondLeash` 의 인스턴스 메모리는 `OnBecomeRelevant` 가 항상 선행 시드한다. `UWxBTTask_Wander` 의 `Bitmask`/`BitmaskEnum` 조합은 `Bitflags` 메타 없이도 에디터가 enum 값을 비트 인덱스로 취급하므로 코드의 `1 << Index` 와 일치한다. `UWxPatrolComponent::GetNextIndex` 의 세 MoveMode 경계값과 `FindPatrolComponent` 의 Owner-우선 조회(스포너가 `SpawnActorDeferred` 로 Owner 를 먼저 세팅함을 확인). 권한 모델: 퍼셉션·BT·소음 보고 모두 서버 전용이고 `UWxAnimNotify_ReportNoise` 는 `HasAuthority` 게이팅, 인식은 MinimalReplication 태그로만 클라에 전파 — 권위 위반 없음.
- **규칙 준수 확인(위반 0건)**: 29개 소스 전부 첫 줄 저작권 표기 정상 / `Wx` prefix 전면 준수 / `BlueprintCallable` 0건 / `FORCEINLINE`·헤더 인라인 정의 0건 / 람다 0건 / 델리게이트 콜백 3종(`HandleTargetPerceptionUpdated`, `HandleDeathTagChanged`, `HandleAbilityEnded`) 모두 `Handle` prefix / void 라이프사이클 override 의 `Super::` 호출 누락 0건 / Wx 플러그인 의존은 `WxCore` 하나(`.Build.cs`·`.uplugin` 동일)
- **미검토 / 한계**:
  - BehaviorTree/Blackboard `.uasset` 자체는 보지 않았다. README 가 규정한 "BeyondLeash 의 FlowAbortMode = Lower Priority", "복귀 브랜치가 전투 브랜치보다 상위 우선순위", "Blackboard 에셋에 5개 키가 동명·동타입 등록" 같은 에셋 측 규약이 실제로 지켜지는지는 C++ 만으로 확인 불가다. 특히 3·4·7번의 실제 체감 크기는 현재 BT 가 조건 데코에 FlowAbortMode 를 걸어 쓰는지에 달려 있다.
  - `WxBlackboardKeys` 의 비-Shipping 진단(`VerifyBlackboardKey`)이 매 accessor 호출마다 `GetKeyID` 이름 선형 탐색을 도는 비용은 인지했으나(`UWxBTDecorator_BeyondLeash::TickNode` 가 매 프레임 호출한다) 실측하지 않아 지적에 포함하지 않았다. 고칠 여지는 `OnBecomeRelevant` 에서 `HomeLocation` 을 노드 메모리에 1회 캐시하는 것이다.
  - `NavigationSystem`·`GameplayTasks` 모듈 의존은 WxAI 소스에서 직접 쓰이지 않으나 `AIModule` 의 전이 의존이라 무해해 지적하지 않았다.
  - 멀티플레이 실환경(전용 서버) 실행 검증, 리시 왕복·타겟 진동의 실제 플레이 확인은 정적 분석 범위 밖이다.

---
*문서 기준 커밋 `1e9b745c` · 리뷰일 2026-08-05 · 소스 29파일 — `/module-review`로 갱신*
