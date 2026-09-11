# WxAI — 코드 리뷰

> 지난 리뷰의 🔴(RandomChoice 가 후보 0일 때 실패를 전파하지 못하던 문제)가 실제로 고쳐졌고, 남은 것은 이월된 설계 이슈 둘과 저작 편의 흠 둘뿐이다. 상태 소유권·수명주기 처리는 여전히 저장소에서 가장 정돈된 편이며 규칙 위반은 0건이다. 커버리지: 소스 38파일 전부와 `Build.cs`·`uplugin`·`README` 를 읽었고, RandomChoice 의 탐색 결과 전파, 어빌리티 발동/중단/종료 프로토콜, `ForgetActor` 이후 재감지 경로, BeyondLeash 의 관찰자 등록 조건을 UE 5.8 엔진 소스(`UBTCompositeNode::FindChildToExecute`/`GetNextChild`/`NotifyDecoratorsOnFailedActivation`, `UBehaviorTreeComponent::OnTaskFinished`/`OnTreeFinished`/`StopTree`/`IsExecutingBranch`/틱 루프, `UAIPerceptionComponent::GetCurrentlyPerceivedActors`/`ForgetActor`/`ForgetAll`, `FActorPerceptionInfo::HasAnyCurrentStimulus`, `UBTNode::GetNodeName`)와 직접 대조해 깊게 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 타겟 승계가 "가장 가까운 적" 이 아니라 해시 순서로 아무나 고른다 (미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:51-66` (호출부 `:48`, 감각 수명은 `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:33-34`)
- **범주**: 버그/정확성
- **문제**: `GetCurrentlyPerceivedActors(nullptr, ...)` 결과를 앞에서부터 훑어 **첫 번째** 유효 액터를 승계한다. 엔진 원본은 `PerceptualData`(`TMap`)를 순회해 담으므로 순서가 해시 버킷 순서이며 거리·최신성·감각 종류와 무관하다. 게다가 `SenseToUse == nullptr` 이면 엔진은 `FActorPerceptionInfo::HasAnyCurrentStimulus()` 로 판정하는데, 이는 "성공 자극이 있고 아직 만료되지 않았다" 는 뜻이라 Hearing·Damage 의 `MaxAge` 5초 동안은 계속 후보로 남는다.
  구체적 실패: 눈앞의 A와 5초 안에 소리를 냈던 먼 곳의 B가 모두 후보일 때, 타겟이던 C가 죽으면 해시 순서에 따라 보이지도 않는 B를 승계할 수 있다. 그러면 `UWxBTService_LockOn` 이 벽 너머를 겨누고 `TargetDistance` 는 사거리 밖 값이 되어 전투 브랜치가 헛돈다. 스폰 순서·해시 배치에 따라 실행마다 달라져 재현도 어렵다.
- **제안**: 루프를 "첫 유효 액터 반환" 대신 "유효 후보 중 폰과의 거리제곱 최소" 로 바꾼다. 이미 가진 목록을 다시 읽는 것뿐이라 추가 감지 비용이 없다. 시야를 우선하고 싶다면 `GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), ...)` 를 먼저 시도하고 비면 전체 목록으로 폴백하는 2단계도 가능하다.
- **확신도**: 중간

### 2. 🟡 어빌리티 발동/중단/종료 프로토콜이 두 태스크에 통째로 복제돼 있다 (미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:41-95`, `:102-139`, `:141-192` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:72-125`, `:141-177`, `:249-299` (상태 필드도 `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h:42-63` ↔ `Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h:65-89` 로 동일)
- **범주**: 중복/복잡도
- **문제**: 발동 대상 태그를 어디서 얻느냐(저작값 `AbilityTag` vs 대상 ASC 폴링 `MirroredTag`)만 다르고, 종료 콜백 선바인드 → `FScopedAbilityListLock` 후보 순회 → `ActivatedHandle` 선기록 → 재발동 판별(`FindAbilitySpecFromHandle` + `IsActive`) → `ActivationResult` 되감기 → `CanBeCanceled` 거부 시 즉시 마감 → `GetTaskStatus` 로 abort/완료를 가르는 종료 처리까지 약 150줄이 로그 문구와 플래그 이름(`bIsRequestingAbort` / `bIsRequestingCancel`)을 빼면 문자 단위로 같다.
  이 프로토콜은 모듈에서 가장 미묘한 코드이고, 실제로 과거에 "`MirrorAbility` 쪽에만 `CanBeCanceled` 가드가 없다" 는 결함이 정확히 이 복제 구조에서 나왔다. 지금도 한쪽만 고칠 위험이 그대로 남아 있으며, 코드 어디에도 두 파일이 쌍이라는 표시가 없다.
- **제안**: 두 노드를 통합하지 말고(별도 클래스 유지는 선행 결정), 발동 대상 태그만 순수 가상으로 뽑은 공통 베이스(`UWxBTTask_AbilityBase` 등)로 발동·중단·종료 구간만 끌어올린다. 구체 BT Task 를 상속하는 것이 아니라 프로토콜 한 벌을 공유하는 것이라 기존 결정과 충돌하지 않는다. 그마저 원치 않으면 최소한 양쪽 헤더에 "이 프로토콜은 반대편 태스크와 쌍으로 유지한다" 는 주석을 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 두 태스크를 독립 클래스로 유지하기로 한 선행 결정이 있다)

### 3. 🟢 BT 노드 3종에 `NodeName` 이 없어 그래프에 클래스명이 그대로 노출된다 (4회 연속 미해결 이월)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:11-14`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:13-19`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-16`
- **범주**: 설계/구조
- **문제**: 나머지 노드는 생성자에서 `NodeName` 을 지정해 BT 에디터에 "Patrol", "Return Home", "Lock On", "Mirror Movement", "Mirror Ability", "Random Choice", "Random Weight", "Beyond Leash", "Update Target Actor", "Update Target Distance" 로 뜨는데 이 셋만 비어 있다. `UBTNode::GetNodeName()` 이 `NodeName.Len() ? NodeName : GetShortTypeName(this)` 이므로 폴백 타입명이 그대로 보인다. 이 모듈은 BT 저작 표면 그 자체라 일관성이 곧 사용성이다.
- **제안**: 각 생성자에 `NodeName = TEXT("Activate Ability")` / `TEXT("Wander")` / `TEXT("Attribute Ratio")` 한 줄씩 추가한다.
- **확신도**: 높음

### 4. 🟢 RandomChoice 에서 "조건은 전원 통과, 가중치만 전원 0" 이면 아무 흔적 없이 브랜치가 멈춘다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89-107` (계약 선언은 `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:26`)
- **범주**: 버그/정확성
- **문제**: 지난 리뷰의 🔴 는 "조건에 막힌 자식 하나를 엔진에 되돌려 준다"(`:101-104`) 는 최소 수정으로 해결됐다 — 엔진 `FindChildToExecute` 가 그 자식의 조건을 다시 검사해 실패시키면서 `LastResult = Failed` 를 세팅하는 것을 엔진 소스로 확인했다. 남은 구멍은 되돌려줄 자식이 없는 경우다: 자식이 0개이거나(`:44-46`), 조건은 전원 통과했는데 가중치가 전원 0이면 `ReturnToParent` 만 반환하고 탐색 결과는 손대지 못해 부모 Selector 가 직전 결과를 그대로 이어받고 폴백 형제가 실행되지 않는다.
  `UWxBTDecorator_RandomWeight::Weight` 의 툴팁이 "0 이면 추첨에서 제외된다"(`Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`)라 디자이너가 공격 묶음을 잠깐 꺼 두려고 전부 0으로 두는 저작이 자연스럽고, 그때 폰이 아무 것도 하지 않는데 로그 한 줄 남지 않는다. 헤더는 이 한계를 이미 명시하고 있으므로 동작 자체는 알려진 것이고, 문제는 저작 실수가 드러나지 않는다는 점이다.
- **제안**: `Candidates.Num() == 0 && BlockedChild == INDEX_NONE` 경로에 `LogWxAI` 경고를 남긴다(어빌리티 태스크들이 `CanBeCanceled` 거부를 경고로 드러내는 것과 같은 방식). 가중치 전원 0을 "가중치 없음(전원 균등)" 으로 해석하는 쪽은 헤더 계약과 어긋나므로 택하지 않는 편이 낫다.
- **확신도**: 높음(동작), 낮음(경고를 남길 가치가 있는지는 판단 문제)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 Public 헤더 19종, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, 소비자 확인용 `Source/WxGame/Controller/WxAIController.cpp`, 태그 계약 확인용 `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`
- **이번에 재판정해 **철회**한 지난 리뷰 항목**: "리시 복귀의 `ForgetActor` 한 번이 사실상 무의미하다"(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:30-40`)는 성립하지 않는다. `UAIPerceptionComponent::ForgetActor` 는 `PerceptualData` 항목만 지우는 것이 아니라 `AIPerceptionSys->OnListenerForgetsActor` 로 각 센스의 이전 판정까지 되돌리므로, (1) 그 대상의 Hearing·Damage 잔여 자극(최대 5초)이 함께 사라지고 (2) Sight 는 다음 성공 질의에서 새 자극을 다시 만든다. 즉 헤더(`WxBTTask_ReturnHome.h:13`)가 선언한 "새 자극을 억제하지 않아 정상 경로로 재획득" 과 코드가 일치한다.
- **미검토 / 한계**:
  - 규칙 위반은 0건이다 — 38파일 전부 저작권 첫 줄과 `Wx` 접두사, `BlueprintCallable`·`FORCEINLINE`·인라인 함수 정의·람다 전부 0건, 델리게이트 콜백(`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded` 2종) 모두 `Handle` 접두사, 의존 모듈은 `WxCore` 하나(+엔진 `AIModule`·`GameplayAbilities`·`NavigationSystem`)이고 모듈 밖 include 도 `WxGameplayTags.h` 하나뿐이다. BT SimpleParallel 사용도 없고 리쉬 판정/실행이 Decorator+Task 로 갈려 있어 프로젝트 AI 원칙과도 일치한다.
  - BT/Blackboard 에셋이 이 노드들을 어떤 트리 형태로 조립하는지는 범위 밖이라, 발견 1·4의 실제 노출 빈도는 코드 근거로만 적었다.
  - `UWxBTTask_MirrorAbility::TickTask`(`:220`)가 `UWxBTTask_ActivateAbility::HandleAbilityEnded`(`:174-180`)와 달리 `EBTTaskStatus::Aborting` 을 보지 않는 비대칭은 확인했으나 발견으로 올리지 않았다 — 엔진이 Aborting 중에도 태스크를 틱하는 것은 맞지만(`UBehaviorTreeComponent::TickComponent` 의 "tick active task"), `OnTaskFinished` 가 `bWasAborting` 을 보고 `RequestExecution` 을 건너뛰므로 `FinishLatentTask(Succeeded)` 가 `FinishLatentAbort()` 와 동일하게 동작한다. 결과값 차이는 디버거 표시와 `ConditionalNotifyChildExecution` 에만 남는데 이 프로젝트는 SimpleParallel 을 쓰지 않는다.
  - `UWxBTTask_Patrol` 이 완주 후 `Super::ExecuteTask` 없이 `InProgress` 로 상주할 때 `UBTTask_MoveTo` 의 노드 메모리(`MoveRequestID` 등)가 직전 실행값으로 남는 점은 이번에도 확인했으나, `AbortTask` 가 요청 ID 불일치를 `AbortMove` 에 그대로 넘기고 `UPathFollowingComponent` 가 이를 무시하므로 실해 시나리오를 만들지 못해 발견으로 올리지 않았다.
  - RandomChoice 가 사전 필터에서 `NotifyDecoratorsOnFailedActivation` 을 부른 뒤 엔진이 `BlockedChild` 에 대해 한 번 더 부르는 이중 통지는 확인했으나, `SearchData.AddUniqueUpdate` 가 중복을 걸러 주고 스톡 데코레이터 중 `OnNodeProcessed` 를 구현한 것은 `UBTDecorator_ForceSuccess` 뿐이라 실해가 없다고 보고 제외했다.
  - 리플리케이션·데디케이티드 서버 경로는 `UWxAnimNotify_ReportNoise` 의 `HasAuthority` 가드와 "퍼셉션/BT 는 서버 전용" 전제 외에는 보지 않았다.
  - `UWxBTService_MirrorMovement` 의 매 프레임 `Trail.RemoveAt(0)` 비용과 `WxBlackboardKeys` accessor 가 비-Shipping 빌드에서 매 호출 `GetKeyID` 선형 탐색을 도는 비용은 규모를 계산해 무해하다고 판단하고 발견에서 제외했다.

---
*문서 기준 커밋 `4a8e5d4b` · 리뷰일 2026-09-11 · 소스 38파일 — `/module-review`로 갱신*
