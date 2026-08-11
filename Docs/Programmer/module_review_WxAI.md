# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹힌 구조가 잘 지켜지고 있고, 수명주기(타겟 소실 구독 해제, GE 핸들 정리, 어빌리티 종료 델리게이트 해제)와 CLAUDE.md 코딩·모듈 규칙은 위반이 없다. 남은 문제는 대부분 "문서가 약속한 시멘틱을 코드가 보장하지 못하는" 지점이다. 이번 리뷰는 `Plugins/WxAI` 전체 29개 소스를 읽고, Perception·RandomChoice·BeyondLeash/ReturnHome·ActivateAbility·Patrol/Wander를 UE 5.8 엔진 원본(`BTCompositeNode.cpp`, `BTDecorator.h`, `AIPerceptionComponent.cpp`, `GameplayEffectTypes`)과 대조해 깊게 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 RandomChoice 의 "후보 없음 = 실패" 시멘틱이 실제로 보장되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:88-92` (문서화된 약속은 `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:23`)
- **범주**: 버그/정확성
- **문제**: 유효 후보가 0이면 `BTSpecialChild::ReturnToParent` 만 반환하는데, `GetNextChildHandler` 는 `LastResult` 를 값으로 받으므로 결과를 바꿀 수 없다. 엔진 `UBTCompositeNode::FindChildToExecute` 는 **자식 Decorator 검사에 실제로 진입했을 때만** `LastResult = EBTNodeResult::Failed` 를 쓴다(UE 5.8 `BTCompositeNode.cpp:54`). 후보가 0이면 루프 본문이 한 번도 돌지 않으므로 유입된 결과가 그대로 부모로 흘러간다.
  실패 시나리오: `Sequence[ RandomChoice(공격패턴), Wait ]` 에서 직전 태스크가 Succeeded 로 끝나 `ContinueWithResult = Succeeded` 인 상태로 검색이 들어오면, 모든 패턴이 조건 필터/가중치 0으로 걸러져도 부모 Sequence 는 "RandomChoice 성공" 으로 보고 다음 자식(Wait)으로 진행한다. `Selector[ RandomChoice, 폴백 ]` 배치에서는 더 나쁘다 — `UBTComposite_Selector::GetNextChildHandler` 는 `LastResult == Failed` 일 때만 다음 자식으로 넘어가므로, 폴백 브랜치가 통째로 건너뛰어지고 AI 가 그 프레임에 아무 행동도 고르지 못한다.
- **제안**: (a) 후보가 0이면서 자식이 존재할 때 Decorator 로 막힌 자식 인덱스 하나를 반환해 엔진 자신의 실패 경로(`LastResult = Failed` → 재질의 → ReturnToParent)를 타게 하거나, (b) 그게 불가능한 "전원 가중치 0" 케이스까지 덮으려면 헤더의 약속을 실제 동작(유입 결과를 그대로 전파)으로 정정하고, 폴백이 필요한 배치에는 항상 실패하는 자식을 마지막에 두도록 규약화한다.
- **확신도**: 중간 (엔진 코드 추적 기반, 런타임 재현은 미확인)

### 2. 🟡 BeyondLeash 의 "FlowAbortMode Self/Both 금지" 가 문서로만 강제된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:11-21`, 제약 서술은 `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:22-23` 및 `Plugins/WxAI/README.md:37`
- **범주**: 설계/구조
- **문제**: 생성자는 기본값만 `EBTFlowAbortMode::LowerPriority` 로 두고, 헤더는 Self/Both 를 선택하면 "복귀가 경계에서 끊기고 재-어그로가 나 경계에서 왕복" 한다고 명시한다. 그러나 `UBTDecorator` 는 `bAllowAbortNone` / `bAllowAbortLowerPri` / `bAllowAbortChildNodes` (UE 5.8 `BTDecorator.h:69-76`)로 에디터 드롭다운을 좁힐 수 있는데 셋 다 기본값 true 로 남아 있다. 즉 디자이너가 BT 에디터에서 Self/Both 를 그냥 고를 수 있고, 그 결과는 조용한 거동 붕괴로만 드러난다.
- **제안**: 생성자에서 `bAllowAbortChildNodes = false;` (Self/Both 차단) 와 `bAllowAbortNone = false;` (None 차단)를 설정해 엔진이 잘못된 선택 자체를 막게 한다.
- **확신도**: 높음

### 3. 🟡 ActivateAbility 가 동기 종료 어빌리티를 항상 Failed 로 보고한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:64-68`
- **범주**: 버그/정확성
- **문제**: `TryActivateAbility` 가 true 를 반환했더라도 어빌리티가 그 안에서 이미 끝났으면(즉발 버프, 몽타주 없는 원샷 등) `ActiveSpec->IsActive()` 가 false 라 `EBTNodeResult::Failed` 를 반환한다. BT 는 무한 InProgress 를 피하려는 방어인데, 정상적으로 실행이 완료된 즉발 어빌리티까지 "실패" 로 뭉뚱그린다 — Sequence 배치에서는 후속 노드가 실행되지 않고, Selector 배치에서는 불필요한 폴백이 발동한다.
- **제안**: `OnAbilityEnded` 델리게이트를 `TryActivateAbility` **이전에** 바인드하고, 콜백이 이미 왔는지(핸들 일치 + 플래그)로 동기 종료를 구분해 `bWasCancelled` 에 따라 Succeeded/Failed 를 반환한다. 그러면 InProgress 고착 방어와 즉발 성공 보고를 동시에 만족한다.
- **확신도**: 중간 (즉발 어빌리티를 AI 가 실제로 쓰는지에 따라 체감 영향이 갈린다)

### 4. 🟢 Wander 가 내비게이션을 우회해 폰을 네비메시 밖으로 밀어낼 수 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:101`
- **범주**: 버그/정확성
- **문제**: `AddMovementInput` 은 CMC 충돌만 볼 뿐 네비메시를 보지 않으므로, 배회 방향에 절벽·비-네비 지형이 있으면 폰이 경로망 밖으로 나간다. 같은 모듈의 Patrol/ReturnHome 은 `UBTTask_MoveTo` 기반이라 경로 탐색이 필요하고, 시작점 투영에 실패하면 복귀가 지속 실패할 수 있다.
- **제안**: 필요하다면 틱마다 `UNavigationSystemV1::ProjectPointToNavigation` 으로 다음 위치를 검증하고 벗어나면 조기 Succeeded 로 끊는다. 배회 거리(기본 1초 × 0.3배속)가 짧아 감수 가능하다고 판단했다면 그 판단을 헤더 주석에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 SetRecognized 가 공유 태그 컨테이너를 자기 상태 기록으로 쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:114-127`
- **범주**: 설계/구조
- **문제**: "이미 발행했는가" 판정을 `ASC->HasMatchingGameplayTag(State_InCombat)` 로 한다. 이 컨테이너는 GE·어빌리티 ActivationOwnedTags·loose 태그가 모두 합산되는 공유 상태다. 현재 프로젝트에서 `State.InCombat` 발행자는 이 컴포넌트 하나뿐이라(전 코드 검색 결과) 지금은 문제가 없지만, 나중에 "전투 태세" GE 등 다른 발행자가 생기면 (a) 인식 on 시 이미 태그가 있어 자기 카운트를 올리지 못하고, (b) 인식 off 시 남의 태그에 대해 `RemoveMinimalReplicationGameplayTag` 를 호출해 GAS 가 `FMinimalReplicationTagCountMap::RemoveTag ... wasn't in the tag map` 에러를 뱉으며 남의 태그를 지운다.
- **제안**: 자기 발행 여부를 컴포넌트 멤버 bool 로 들고 그것으로 전환을 판정한다(태그 조회는 제거).
- **확신도**: 낮음(현재는 단독 발행자라 잠재 위험)

### 6. 🟢 Build.cs 에 사용하지 않는 모듈 의존이 남아 있다
- **위치**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs:19-20`
- **범주**: 중복/복잡도
- **문제**: `GameplayTasks` 와 `NavigationSystem` 은 모듈 전체 소스에서 단 한 번도 include·참조되지 않는다(전 파일 검색 결과 0건). 둘 다 `AIModule` 이 이미 Public 의존으로 끌어오므로 순수 잉여다.
- **제안**: 두 항목을 제거한다. 남는 `AIModule`/`GameplayAbilities`/`GameplayTags` 는 Public 헤더에서 실제로 쓰이므로 Public 유지가 맞다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h`, 나머지 Public 헤더 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 소비 측 대조용 `Source/WxGame/Controller/WxEnemyController.cpp`
- **미검토 / 한계**:
  - BT/BB/BP 에셋 내부 구성(각 노드의 FlowAbortMode 실제 값, Blackboard 키 등록 여부, `MoveSpeedEffect` 지정 여부)은 범위 밖이라 확인하지 않았다. 발견 1·2 의 실제 발현 여부는 에셋 배치에 달려 있다.
  - 검증 후보로 파고들었으나 **문제 없음**으로 결론 낸 항목(재확인 불필요): `FWxBTRandomChoiceMemory` 의 `FBTCompositeMemory` 상속 + `GetInstanceMemorySize` 오버라이드는 엔진 `FBTParallelMemory` 와 동일한 정상 패턴이고, `bCreateNodeInstance=true` + `UBTTask_MoveTo` 조합도 엔진 `FindInstanceContainingNode`/`FindTemplateNode` 가 인스턴스를 정규화하므로 안전하다. 생성자에서의 `AddDynamic`·`ConfigureSense` 중복 등록도 엔진이 각각 처리한다(`ConfigureSense` 는 클래스 기준 dedupe). 타겟 사망/파괴 콜백 안에서의 델리게이트 해제도 멀티캐스트 브로드캐스트 중 제거로 안전하다. `GetLocationAtSplinePoint` 는 인덱스를 clamp 하므로 커서 범위 초과로 원점 이동이 나지 않는다.
  - CLAUDE.md 코딩·모듈 규칙(Copyright 첫 줄, Wx prefix, `Handle` prefix, `BlueprintCallable`, 람다, `FORCEINLINE`, WxCore 외 Wx 플러그인 참조)은 전 파일 기계 검사로 위반 0건을 확인했다.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 29파일 — `/module-review`로 갱신*
