# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹는 설계 의도가 잘 지켜져 있고, 수명주기 관리(타겟 소실 구독 해제, 감속 GE 핸들 정리, 어빌리티 종료 델리게이트 해제)와 CLAUDE.md 코딩·모듈 규칙은 위반이 없다. 남은 문제는 대체로 "헤더가 약속한 시멘틱을 엔진 메커니즘이 실제로는 제공하지 않는" 지점에 몰려 있다. 이번 리뷰는 `Plugins/WxAI` 전체 29개 소스를 읽고, Perception·RandomChoice·BeyondLeash/ReturnHome·AttributeRatio·ActivateAbility·Patrol/Wander 를 UE 5.8 엔진 원본(`BTCompositeNode.cpp`, `BehaviorTreeComponent.cpp`, `BTDecorator.h`, `BTTask_MoveTo.cpp`, `SplineComponent.cpp`, `AbilitySystemComponent.h`)과 대조해 깊게 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 AttributeRatio 가 안내하는 "FlowAbortMode 실시간 재평가" 가 실제로는 동작하지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:16` (구현부 `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-13`)
- **범주**: 버그/정확성
- **문제**: 헤더는 "실시간 재평가가 필요한 경우 BT 에디터에서 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다" 고 안내하지만, 이 데코레이터는 생성자에서 알림 플래그를 켜지 않고 `OnBecomeRelevant`/`TickNode` 도 오버라이드하지 않는다. UE 5.8 에서 FlowAbortMode 는 abort 의 *범위*만 정할 뿐 재평가를 **촉발하지 않는다** — 촉발은 누군가 `RequestExecution` 을 불러야 하고, 엔진 Blackboard 계열은 `UBTDecorator_BlackboardBase::OnBecomeRelevant` 에서 키 옵저버를 등록하고(`BTDecorator_BlackboardBase.cpp:34-40`), `UBTDecorator_ConeCheck` 는 `TickNode` 폴링 후 `RequestExecution` 을 호출한다(`BTDecorator_ConeCheck.cpp:107-115`). 어트리뷰트는 Blackboard 키가 아니라 관찰할 대상이 없는데 폴링도 없다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 가 정확히 이 이유로 TickNode 폴링을 직접 구현한 것과 대비된다.
  실패 시나리오: 디자이너가 "HP <= 0.3" 데코에 FlowAbortMode=Self 를 걸어 보스가 패턴 도중 즉시 광폭화 브랜치로 넘어가길 기대해도, 현재 브랜치가 자연 종료돼 다음 탐색이 돌 때까지 아무 일도 일어나지 않는다. 실패가 조용해서(에러·경고 없음) 원인 추적이 어렵다.
- **제안**: BeyondLeash 와 같은 형태로 `INIT_DECORATOR_NODE_NOTIFY_FLAGS()` + 인스턴스 메모리에 직전 판정 저장 + `TickNode` 에서 값이 바뀔 때만 `RequestExecution(this)` 를 넣는다. 실시간이 필요 없다는 판단이면(주 용도가 RandomChoice 후보 필터라면) 헤더 16행 안내를 "탐색 시점 조건 전용, 실시간 abort 불가" 로 정정한다.
- **확신도**: 높음

### 2. 🟡 RandomChoice 의 "후보 없음 = 실패" 시멘틱이 실제로 보장되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:88-92` (문서화된 약속은 `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:23`)
- **범주**: 버그/정확성
- **문제**: 유효 후보가 0이면 `BTSpecialChild::ReturnToParent` 만 반환하는데, `GetNextChildHandler` 는 `LastResult` 를 값으로 받으므로 결과를 바꿀 수 없다. 엔진 `UBTCompositeNode::FindChildToExecute` 는 **자식 Decorator 검사 루프 본문에 진입했을 때만** `LastResult = EBTNodeResult::Failed` 를 쓰고(UE 5.8 `BTCompositeNode.cpp:54`), 후보가 0이면 루프가 한 번도 돌지 않으므로 유입된 `NodeResult` 가 그대로 부모로 흘러간다(`BehaviorTreeComponent.cpp:2097-2105`).
  실패 시나리오: `Sequence[ RandomChoice(공격패턴), Wait ]` 에서 직전 태스크가 Succeeded 로 끝나 `ContinueWithResult = Succeeded` 인 상태로 검색이 들어오면, 모든 패턴이 조건 필터·가중치 0으로 걸러져도 부모 Sequence 는 "RandomChoice 성공" 으로 보고 다음 자식(Wait)으로 진행한다. `Selector[ RandomChoice, 폴백 ]` 배치는 더 나쁘다 — `UBTComposite_Selector::GetNextChildHandler` 는 `LastResult == Failed` 일 때만 다음 자식으로 넘어가므로 폴백 브랜치가 통째로 건너뛰어진다.
- **제안**: (a) 후보가 0이고 자식이 존재할 때 Decorator 로 막힌 자식 인덱스 하나를 반환해 엔진 자신의 실패 경로(`LastResult = Failed` → 재질의 → ReturnToParent)를 타게 하거나, (b) "전원 가중치 0" 케이스까지 덮으려면 헤더의 약속을 실제 동작(유입 결과 전파)으로 정정하고 폴백이 필요한 배치에는 항상 실패하는 자식을 마지막에 두도록 규약화한다.
- **확신도**: 중간 (엔진 코드 추적 기반, 런타임 재현은 미확인)

### 3. 🟡 BeyondLeash 의 "FlowAbortMode Self/Both 금지" 가 문서로만 강제된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:11-21` (제약 서술은 `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:22-23`, `Plugins/WxAI/README.md:37`)
- **범주**: 설계/구조
- **문제**: 생성자는 기본값만 `EBTFlowAbortMode::LowerPriority` 로 두는데, 헤더는 Self/Both 를 고르면 "복귀가 경계에서 끊기고 재-어그로가 나 경계에서 왕복" 한다고 명시한다. `UBTDecorator` 는 `bAllowAbortNone`/`bAllowAbortLowerPri`/`bAllowAbortChildNodes`(UE 5.8 `BTDecorator.h:70-76`)로 에디터 드롭다운 자체를 좁힐 수 있는데 셋 다 기본 true 로 남아 있어, 디자이너가 Self/Both 를 그냥 고를 수 있고 결과는 조용한 거동 붕괴로만 드러난다.
  참고: LowerPriority 로 유지되는 한 설계는 정확하다 — 엔진이 브랜치 활성화 시 LowerPriority 데코의 aux 등록을 해제하므로(`BTCompositeNode.cpp:227-250`) 복귀 중에는 TickNode 폴링 자체가 멈추고 자기중단이 일어날 여지가 없다. 즉 이 방어는 오직 에디터에서의 오설정만 막으면 된다.
- **제안**: 생성자에 `bAllowAbortChildNodes = false;`(Self/Both 차단)와 `bAllowAbortNone = false;`(None 차단)를 추가해 엔진이 잘못된 선택 자체를 막게 한다.
- **확신도**: 높음

### 4. 🟡 ActivateAbility 가 동기 종료 어빌리티를 항상 Failed 로 보고한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:64-68`
- **범주**: 버그/정확성
- **문제**: `TryActivateAbility` 가 true 를 반환했더라도 어빌리티가 그 안에서 이미 끝났으면(즉발 버프, 몽타주 없는 원샷 등) `ActiveSpec->IsActive()` 가 false 라 `EBTNodeResult::Failed` 를 반환한다. 무한 InProgress 고착을 막으려는 방어인데, 정상 완료된 즉발 어빌리티까지 "실패" 로 뭉뚱그린다 — Sequence 배치에선 후속 노드가 실행되지 않고, Selector 배치에선 불필요한 폴백이 발동한다.
- **제안**: `OnAbilityEnded` 델리게이트를 `TryActivateAbility` **이전에** 바인드하고, 콜백이 이미 도착했는지(핸들 일치 + 플래그)로 동기 종료를 판별해 `bWasCancelled` 에 따라 Succeeded/Failed 를 반환한다. 고착 방어와 즉발 성공 보고를 동시에 만족한다.
- **확신도**: 중간 (즉발 어빌리티를 AI 가 실제로 쓰는지에 따라 체감 영향이 갈린다)

### 5. 🟢 RandomChoice 의 사전 필터가 탈락 자식 Decorator 의 관찰자 등록을 건너뛴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:57-64`
- **범주**: 설계/구조
- **문제**: 엔진 `FindChildToExecute` 는 Decorator 에 막힌 자식마다 `NotifyDecoratorsOnFailedActivation` 을 호출해 LowerPriority/Both 데코를 aux 노드로 등록한다(UE 5.8 `BTCompositeNode.cpp:297-320`). RandomChoice 는 `GetNextChildHandler` 에서 후보를 미리 걸러 당첨 인덱스만 돌려주므로, 엔진은 그 자식 하나만 검사하고 탈락 자식의 데코들은 등록 통지를 받지 못한다. 결과적으로 탈락 자식에 붙은 관찰형 데코(엔진 Blackboard 데코 포함)는 조건이 참으로 바뀌어도 스스로 브랜치를 깨우지 못한다. 발견 1과 합치면 "RandomChoice 아래 조건 데코는 진입 시점에만 평가된다" 가 실질 규약이다.
- **제안**: "한 진입에서 자식 1개" 시멘틱상 의도한 결과라면, 헤더와 README 확장 포인트 절에 "RandomChoice 자식에 붙인 Decorator 의 FlowAbortMode 는 동작하지 않는다(진입 시점 필터 전용)" 를 명시해 오설정을 예방한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 SetRecognized 가 공유 태그 컨테이너를 자기 상태 기록으로 쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:114-127`
- **범주**: 설계/구조
- **문제**: "이미 발행했는가" 판정을 `ASC->HasMatchingGameplayTag(State_InCombat)` 로 한다. 이 컨테이너는 GE·어빌리티 ActivationOwnedTags·loose 태그가 합산되는 공유 상태다. 현재 `State.InCombat` 발행자는 이 컴포넌트 하나뿐이라(전 코드 검색 결과) 동작하지만, 나중에 "전투 태세" GE 등 다른 발행자가 생기면 (a) 인식 on 시 이미 태그가 있어 자기 카운트를 올리지 못하고, (b) 인식 off 시 남의 태그에 대해 `RemoveMinimalReplicationGameplayTag` 를 호출해 `FMinimalReplicationTagCountMap::RemoveTag ... wasn't in the tag map` 에러(UE 5.8 `GameplayEffectTypes.cpp:1624`)를 내며 남의 태그를 지운다.
- **제안**: 자기 발행 여부를 컴포넌트 멤버 bool 로 들고 그것으로 전환을 판정한다(태그 조회 제거).
- **확신도**: 낮음(현재는 단독 발행자라 잠재 위험)

### 7. 🟢 Wander 가 내비게이션을 우회해 폰을 네비메시 밖으로 밀어낼 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:101`
- **범주**: 버그/정확성
- **문제**: `AddMovementInput` 은 CMC 충돌만 볼 뿐 네비메시를 보지 않으므로, 배회 방향에 절벽·비-네비 지형이 있으면 폰이 경로망 밖으로 나간다. 같은 모듈의 Patrol/ReturnHome 은 `UBTTask_MoveTo` 기반이라 경로 탐색이 필요하고, 시작점 투영에 실패하면 이후 복귀가 지속 실패할 수 있다.
- **제안**: 필요하다면 틱마다 `UNavigationSystemV1::ProjectPointToNavigation` 으로 다음 위치를 검증하고 벗어나면 조기 Succeeded 로 끊는다. 배회 거리(기본 1초 × 0.3배속)가 짧아 감수 가능하다고 판단했다면 그 판단을 헤더 주석에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 8. 🟢 Build.cs 에 사용하지 않는 모듈 의존이 남아 있다
- **위치**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs:19-20`
- **범주**: 중복/복잡도
- **문제**: `GameplayTasks` 와 `NavigationSystem` 은 모듈 전체 소스에서 단 한 번도 include·참조되지 않는다(전 파일 검색 결과 0건). 둘 다 `AIModule` 이 이미 Public 의존으로 끌어오므로 순수 잉여다.
- **제안**: 두 항목을 제거한다. 남는 `AIModule`/`GameplayAbilities`/`GameplayTags` 는 Public 헤더에서 실제로 쓰이므로 Public 유지가 맞다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h`, 나머지 Public 헤더 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 소비 측 대조용 `Source/WxGame/Controller/WxEnemyController.cpp`
- **미검토 / 한계**:
  - BT/BB/BP 에셋 내부 구성(각 노드의 FlowAbortMode 실제 값, Blackboard 키 등록 여부, `MoveSpeedEffect` 지정 여부)은 범위 밖이라 확인하지 않았다. 발견 1·2·3·5 의 실제 발현 여부는 에셋 배치에 달려 있다.
  - 검증 후보로 파고들었으나 **문제 없음**으로 결론 낸 항목(재확인 불필요): `FWxBTRandomChoiceMemory` 의 `FBTCompositeMemory` 상속 + `GetInstanceMemorySize` 오버라이드는 엔진 `UBTCompositeNode::GetInstanceMemorySize`(`BTCompositeNode.cpp:702-705`)와 정합하는 정상 패턴이다. `bCreateNodeInstance=true` + `UBTTask_MoveTo` 조합, `bPatrolFinished` 조기 Succeeded 로 `Super::ExecuteTask` 를 건너뛴 뒤의 `UBTTask_MoveTo::OnTaskFinished` 호출도 안전하다(핸들 Reset/Unregister 가 멱등). `UWxBTTask_ActivateAbility` 의 델리게이트는 `StopLogic`/`StopTree` 경로에서도 `WrappedAbortTask` 를 거쳐 해제된다(`BehaviorTreeComponent.cpp:379-397`). BeyondLeash 의 LowerPriority 폴링은 브랜치 활성 시 aux 해제로 자동 정지해 경계 왕복이 생기지 않는다. `AddMinimalReplicationGameplayTag` 는 `UpdateTagMap` 을 함께 호출하므로 현재의 전환 가드는 정상 동작한다. `GetLocationAtSplinePoint` 는 인덱스를 clamp 하므로 정찰 커서 범위 초과로 원점 이동이 나지 않는다. `OnEndPlay` 브로드캐스트 중 `RemoveDynamic` 도 엔진이 호출 목록을 복사해 돌아 안전하다.
  - CLAUDE.md 코딩·모듈 규칙(Copyright 첫 줄, Wx prefix, 델리게이트 콜백 `Handle` prefix, `BlueprintCallable`, 람다, `FORCEINLINE`/인라인 정의, WxCore 외 Wx 플러그인 참조)은 전 파일 기계 검사로 위반 0건을 확인했다.

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 29파일 — `/module-review`로 갱신*
