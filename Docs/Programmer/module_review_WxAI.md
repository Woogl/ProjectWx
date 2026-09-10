# WxAI — 코드 리뷰

> 상태 소유권·수명주기 처리는 여전히 이 저장소에서 가장 잘 정리된 편이고 규칙 위반도 0건이다. 다만 지난 리뷰 이후 퍼셉션에서 BT 서비스로 갈라져 나온 표적 흐름(38파일로 증가)을 다시 검증하면서, `UWxBTComposite_RandomChoice` 가 문서화된 계약("후보 0이면 실패 반환")을 실제로는 지키지 못한다는 새 결함을 찾았다. 커버리지: 소스 38파일 전부와 `Build.cs`·`uplugin`·`README` 를 읽었고, RandomChoice 의 탐색 결과 전파와 어빌리티 발동/중단 프로토콜은 UE 5.8 엔진 소스(`UBTCompositeNode::FindChildToExecute`/`GetNextChild`/`OnNodeActivation`, `UBTComposite_Selector::GetNextChildHandler`, `UBehaviorTreeComponent::ProcessExecutionRequest`/`OnTaskFinished`/`OnTreeFinished`, `UAIPerceptionComponent::ForgetActor`/`ConfigureSense`/`GetCurrentlyPerceivedActors`)를 직접 대조하며 깊게 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 RandomChoice 가 후보 0일 때 실패를 전파하지 못해, 상위 Selector 의 폴백 형제가 영구히 실행되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:91-94` (계약 선언은 `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:24`)
- **범주**: 버그/정확성
- **문제**: 헤더는 "유효 후보가 하나도 없으면 아무 자식도 실행하지 않고 **실패를 반환**한다" 고 선언하지만, 코드는 `BTSpecialChild::ReturnToParent` 만 반환하고 탐색 결과는 건드리지 않는다. 그리고 건드릴 수도 없다 — `GetNextChildHandler(FBehaviorTreeSearchData&, int32, EBTNodeResult::Type LastResult)` 의 세 번째 인자는 **값 전달**이라, 엔진이 `FindChildToExecute(SearchData, EBTNodeResult::Type& LastResult)` 로 참조로 들고 다니는 그 결과 변수에 접근할 방법이 이 훅에는 없다.
  엔진 `UBTComposite_Selector` 는 같은 상황에서 `FindChildToExecute` 의 루프를 실제로 돌면서 자식마다 `LastResult = EBTNodeResult::Failed` 를 세팅하기 때문에 실패가 정상 전파된다. 반면 이 Composite 는 자식을 **사전 필터**하고 첫 호출에서 곧장 `ReturnToParent` 를 반환하므로 그 세팅 지점을 통째로 건너뛴다. 결과적으로 부모는 이 브랜치에 들어오기 전의 결과값을 그대로 다시 읽는다.
  구체적 실패: `Selector[ RandomChoice(공격 A/B/C, 각각 사거리 Decorator), Approach ]` 에서 셋 다 사거리 밖이면 RandomChoice 는 후보 0으로 `ReturnToParent` 를 반환한다. 이때 `NodeResult` 는 `ProcessExecutionRequest` 진입값(`ExecutionRequest.ContinueWithResult`) 그대로이며, 트리 루프 재시작 경로(`OnTreeFinished` → `RequestExecution(..., EBTNodeResult::InProgress)`)와 `SwitchToHigherPriority` 경로에서는 `InProgress`/`Aborted`, 선행 형제가 성공한 Sequence 경로에서는 `Succeeded` 다. Selector 는 `LastResult == Failed` 일 때만 다음 자식으로 넘어가므로 **어느 경우에도 `Approach` 가 선택되지 않는다.** 폰은 사거리 밖에서 아무 것도 하지 않고, 트리는 매 프레임 루트부터 탐색만 되풀이한다.
  이 조합(추첨 공격 묶음 + 사거리 게이트 + 접근 폴백)은 전투 트리의 표준 형태라 노출 빈도가 낮다고 보기 어렵다.
- **제안**: 사전 필터를 버리고 엔진 루프에 판정을 맡긴다 — 무작위로 고른 인덱스를 그대로 반환하면 `FindChildToExecute` 가 `DoDecoratorsAllowExecution` 을 돌려 실패 시 `LastResult = Failed` 세팅과 `NotifyDecoratorsOnFailedActivation` 통지를 대신 해 주고, `GetNextChild(그 인덱스, Failed)` 로 다시 물어 온다. 같은 탐색에서 이미 시도한 자식을 노드 메모리에 누적해, `PrevChild != NotInitialized` 일 때 남은 후보에서 다시 추첨하고 전부 소진되면 `ReturnToParent` 를 반환하면 된다(현재 `WxBTComposite_RandomChoice.cpp:62-66` 의 수동 통지도 함께 사라진다).
  최소 수정만 원하면, 후보가 0이고 데코레이터로 걸러진 자식이 하나 이상일 때 `ReturnToParent` 대신 그 자식 인덱스를 반환한다 — 엔진이 같은 검사를 다시 해 실패시키면서 `LastResult` 를 Failed 로 바꿔 준다. 단 이 경우 "전원 가중치 0" 으로만 후보가 빈 경우는 여전히 남는다.
- **확신도**: 높음 (전파 경로는 엔진 소스로 확인. 실제 체감 강도는 BT 에셋이 RandomChoice 뒤에 폴백 형제를 두는지에 달렸고, 그 부분은 리뷰 범위 밖이다)

### 2. 🟡 타겟 승계가 "가장 가까운 적" 이 아니라 해시 순서로 아무나 고른다 (미해결 이월 — 위치 이동)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:51-66` (호출부 `:48`, 감각 수명은 `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:33-34`)
- **범주**: 버그/정확성
- **문제**: 지난 리뷰 시점엔 `WxAIPerceptionComponent` 에 있던 `FindPerceivedTarget` 이 이 서비스로 옮겨왔지만 로직은 그대로다. `GetCurrentlyPerceivedActors(nullptr, ...)` 결과를 앞에서부터 훑어 **첫 번째** 유효 액터를 승계한다. 원본은 `TMap<TObjectKey<AActor>, FActorPerceptionInfo>` 라 순서가 해시 버킷 순서이며 거리·최신성·감각 종류와 무관하다. 게다가 `SenseToUse == nullptr` 이면 엔진은 `HasAnyCurrentStimulus()` 로 판정하는데, 이는 만료 전 성공 자극이 하나라도 있으면 참이다. Hearing·Damage 의 `MaxAge` 가 5초이므로 "4초 전에 소리를 낸 뒤 사라진 액터" 도 후보가 된다.
  구체적 실패: 눈앞의 A와 5초 안에 소리를 냈던 먼 곳의 B가 모두 후보일 때, 타겟이던 C가 죽으면 해시 순서에 따라 보이지도 않는 B를 승계할 수 있다. 그러면 `UWxBTService_LockOn` 이 벽 너머를 겨누고 `TargetDistance` 는 사거리 밖 값이 되어 전투 브랜치가 헛돈다. 스폰 순서·해시 배치에 따라 실행마다 달라져 재현도 어렵다.
- **제안**: 루프를 "첫 유효 액터 반환" 대신 "유효 후보 중 폰과의 거리제곱 최소" 로 바꾼다. 이미 가진 목록만 다시 읽는 것이라 추가 감지 비용이 없다. 시야를 우선하고 싶다면 `GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), ...)` 를 먼저 시도하고 비면 전체 목록으로 폴백하는 2단계도 가능하다.
- **확신도**: 중간

### 3. 🟡 어빌리티 발동/중단/종료 프로토콜이 두 태스크에 통째로 복제돼 있다 (미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:16-95`, `:102-139`, `:141-192` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:46-125`, `:141-177`, `:249-299` (상태 필드도 `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h:38-63` ↔ `Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h:56-89` 로 동일)
- **범주**: 중복/복잡도
- **문제**: 발동 대상 태그를 어디서 얻느냐(저작값 `AbilityTag` vs 대상 ASC 폴링 `MirroredTag`)만 다르고, `FScopedAbilityListLock` 후보 순회 → `ActivatedHandle` 선기록 → 재발동 판별(`FindAbilitySpecFromHandle` + `IsActive`) → `ActivationResult` 되감기 → `CanBeCanceled` 거부 시 즉시 마감 → `GetTaskStatus` 로 abort/완료를 가르는 종료 처리까지 약 150줄이 주석 문구를 빼면 문자 단위로 같다.
  이 프로토콜은 모듈에서 가장 미묘한 코드이고, 실제로 과거에 "`MirrorAbility` 쪽에만 `CanBeCanceled` 가드가 없다" 는 결함이 정확히 이 복제 구조에서 나왔다. 지금도 한쪽만 고칠 위험이 그대로 남아 있으며, 코드 어디에도 두 파일이 쌍이라는 표시가 없다. 실제 비대칭이 이미 하나 남아 있다 — `MirrorAbility` 의 `TickTask` 마감(`:220`)은 `ActivateAbility` 의 종료 콜백(`:174-180`)처럼 `GetTaskStatus(this) == EBTTaskStatus::Aborting` 을 확인하지 않는다(엔진 `OnTaskFinished` 가 `bWasAborting` 을 보고 결과값을 무시하므로 트리 상태 자체는 복구되어 실질 영향은 디버거 표시에 그친다).
- **제안**: 두 노드를 통합하지 말고(별도 클래스 유지는 선행 결정), 발동 대상 태그만 순수 가상으로 뽑은 공통 베이스(`UWxBTTask_AbilityBase` 등)로 발동·중단·종료 구간만 끌어올린다. 구체 BT Task 를 상속하는 것이 아니라 프로토콜 한 벌을 공유하는 것이라 기존 결정과 충돌하지 않는다. 그마저 원치 않으면 최소한 양쪽 헤더에 "이 프로토콜은 반대편 태스크와 쌍으로 유지한다" 는 주석을 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 두 태스크를 독립 클래스로 유지하기로 한 선행 결정이 있다)

### 4. 🟢 BT 노드 3종에 `NodeName` 이 없어 그래프에 클래스명이 그대로 노출된다 (3회 연속 미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:11-14`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:13-19`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-16`
- **범주**: 설계/구조
- **문제**: 나머지 노드는 생성자에서 `NodeName` 을 지정해 BT 에디터에 "Patrol", "Lock On", "Random Choice", "Mirror Ability", "Mirror Movement", "Beyond Leash", "Return Home", "Random Weight", "Update Target Actor", "Update Target Distance" 로 뜨는데 이 셋만 비어 있다. `UBTNode::GetNodeName()` 이 `NodeName.Len() ? NodeName : GetShortTypeName(this)` 이므로 폴백 타입명이 그대로 보인다. 이 모듈은 BT 저작 표면 그 자체라 일관성이 곧 사용성이다.
- **제안**: 각 생성자에 `NodeName = TEXT("Activate Ability")` / `TEXT("Wander")` / `TEXT("Attribute Ratio")` 한 줄씩 추가한다.
- **확신도**: 높음

### 5. 🟢 리시 복귀가 끝난 뒤에는, 복귀 내내 계속 보이던 적대 액터를 다시 물지 못한다 (미해결 이월 — 위치 이동)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:30-40`
- **범주**: 버그/정확성
- **문제**: 지난 리뷰의 `WxAIPerceptionComponent::ForgetTargetActor` 가 이 태스크 안으로 인라인됐지만 동작은 같다 — **현재 타겟 하나만** `ForgetActor` 하고 `SetTargetActor(nullptr)` 로 끝낸다. 다른 적대 액터의 감지 기록은 그대로 남고, 엔진은 감지 상태가 뒤집힐 때만 새 통지를 만들므로 복귀 내내 계속 보이고 있던 액터는 새 자극을 만들지 않는다. 다만 `UWxBTService_UpdateTargetActor` 는 통지가 아니라 매 틱 `GetCurrentlyPerceivedActors` 를 폴링하므로, 복귀 직후 그 목록에 남아 있는 액터는 다음 틱에 다시 승계된다 — 즉 지난 리뷰가 우려한 "영구 미획득" 은 폴링 전환으로 이미 해소됐고, 남은 것은 `ForgetActor` 한 번이 사실상 무의미해졌다는 점이다(같은 대상이 곧바로 재승계되므로 "복귀 동안 타겟 없음" 도 성립하지 않는다).
- **제안**: 의도가 "복귀 동안은 타겟을 비운다" 라면 `ForgetActor` 한 개로는 부족하므로, 빙의 변경 경로(`WxAIPerceptionComponent.cpp:106`)처럼 `ForgetAll()` 을 쓰거나 복귀 브랜치가 도는 동안 표적 선정 서비스를 게이팅한다. 의도가 "즉시 재승계돼도 무방(브랜치는 `BeyondLeash` 가 잡고 있으므로 전투로 넘어가지 않는다)" 이라면, `ForgetActor` 호출을 지우고 `SetTargetActor(nullptr)` 만 남겨 아무 일도 하지 않는 코드를 없앤다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 어느 쪽이 목적인지는 코드만으로 가릴 수 없다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 Public 헤더 19종, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, 태그 계약 확인용 `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`
- **미검토 / 한계**:
  - 규칙 위반은 0건이다 — 38파일 전부 저작권 첫 줄과 `Wx` 접두사, `BlueprintCallable`·`FORCEINLINE`·인라인 함수 정의·람다 전부 0건, 델리게이트 콜백(`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded` 2종) 모두 `Handle` 접두사, 의존 모듈은 `WxCore` 하나(+엔진 `AIModule`·`GameplayAbilities`·`NavigationSystem`). BT SimpleParallel 사용도 없고 리쉬 판정/실행이 Decorator+Task 로 갈려 있어 프로젝트 AI 원칙과도 일치한다.
  - BT/Blackboard 에셋이 이 노드들을 어떤 트리 형태로 조립하는지는 범위 밖이라, 발견 1의 실제 노출 빈도(RandomChoice 뒤에 폴백 형제를 두는 트리가 존재하는지)와 발견 2·5의 체감 강도는 코드 근거로만 적었다.
  - `UWxBTTask_Patrol` 이 완주 후 `Super::ExecuteTask` 없이 `InProgress` 로 상주할 때 `UBTTask_MoveTo` 의 노드 메모리(`MoveRequestID` 등)가 직전 실행값으로 남는 점은 확인했으나, `UPathFollowingComponent::AbortMove` 가 요청 ID 불일치를 무시하므로 실해가 나는 시나리오를 만들지 못해 발견으로 올리지 않았다.
  - 리플리케이션·데디케이티드 서버 경로는 `UWxAnimNotify_ReportNoise` 의 `HasAuthority` 가드와 "퍼셉션/BT 는 서버 전용" 전제 외에는 보지 않았다.
  - `UWxBTService_MirrorMovement` 의 매 프레임 `Trail.RemoveAt(0)` 비용과 `UWxBTService_UpdateTargetActor` 의 10Hz `TArray` 할당은 규모를 계산해 무해하다고 판단하고 발견에서 제외했다.

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 38파일 — `/module-review`로 갱신*
