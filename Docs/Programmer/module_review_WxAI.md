# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 엔진 BT/Perception 규약을 정확히 따르고, 위험한 지점(노드 메모리 레이아웃, 어빌리티 종료 통지의 재진입, 타겟 소실 구독 해제)마다 근거 주석이 남아 있어 의도가 읽힌다. 이번 리뷰는 `Plugins/WxAI` 의 C++ 29파일 전부를 읽고, 그중 상태·수명주기가 얽힌 `WxAIPerceptionComponent`·`WxBTTask_ActivateAbility`·`WxBTComposite_RandomChoice`·`WxBTTask_Patrol` 은 UE 5.8 엔진 소스와 대조하며 깊게 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 빙의 해제(UnPossess) 경로에서 AI 포커스와 회전 모드가 원복되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:170-176`, `:256-284`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged` 는 `SetTargetActor(nullptr)` 로 타겟을 되돌리지만, `SetTargetActor` 의 원복 구간(`AIC->ClearFocus`, 회전 모드 복원)은 `Cast<ACharacter>(AIC->GetPawn())` 가 유효할 때만 실행된다(`:256-261`). 엔진은 `AController::UnPossess` 에서 `SetPawn(nullptr)` 을 먼저 끝낸 뒤 `OnPossessedPawnChanged` 를 방송하므로(`AController::UnPossess`, Controller.cpp:386-408), 빙의 해제 시점엔 `GetPawn()` 이 이미 null 이라 이 구간이 통째로 건너뛰어진다. 결과는 두 가지다 — (a) 컨트롤러에 `EAIFocusPriority::Gameplay` 포커스가 옛 타겟을 가리킨 채 남고, (b) 이전 폰의 `bUseControllerDesiredRotation=true` / `bOrientRotationToMovement=false` 가 그대로 남는다. 포커스는 폰이 아니라 **컨트롤러** 소유라 다음 빙의까지 살아남고, 그다음 `SetTargetActor(nullptr)` 은 `AppliedTarget` 이 이미 null 이라 `:246-249` 에서 조기 반환하므로 영영 정리되지 않는다. `AWxEnemyController::OnPossess` 주석("재사용된 폰도 새 빙의에서는 타겟 없이 시작한다", `Source/WxGame/Controller/WxEnemyController.cpp:34`)이 폰 재사용 운용을 전제하므로 재현 가능한 경로다. 재빙의된 적이 타겟 없이도 옛 타겟 쪽을 계속 바라보게 된다.
- **제안**: `ClearFocus` 와 회전 모드 복원을 `Character` 유효성 뒤가 아니라 앞에서 처리하거나, `HandlePossessedPawnChanged` 가 `OldPawn` 을 받아 그 폰의 무브먼트에 복원을 적용하도록 분리한다(포커스 해제는 폰과 무관하므로 무조건).
- **확신도**: 중간 — 컨트롤러가 폰과 함께 항상 파괴되는 운용이라면 무해하다.

### 2. 🟡 Patrol 커서 상태가 빙의 교체·경로 변경에서 초기화되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h:52-59`, `Private/WxBTTask_Patrol.cpp:47-50`, `:99-112`
- **범주**: 설계/구조
- **문제**: `PatrolCursor`·`PatrolDirection`·`bPatrolFinished` 는 `bCreateNodeInstance` 노드 인스턴스에 담겨 **BT 컴포넌트(=컨트롤러) 수명 동안** 유지되는데, 이를 되돌리는 지점이 어디에도 없다. 헤더 주석은 "폰마다 독립된 커서"라고 하지만 실제 소유자는 폰이 아니라 컨트롤러다. 두 가지 상황에서 어긋난다. (a) 컨트롤러가 다른 폰(=다른 스포너, 다른 경로)을 다시 빙의하면 이전 경로 기준 커서·방향이 그대로 이어진다. (b) `bPatrolFinished` 는 한 번 켜지면 절대 꺼지지 않아, 이후 어떤 경로를 받아도 `ExecuteTask` 가 `:47-50` 에서 곧장 `InProgress` 를 반환하며 브랜치를 영구 점유한다 — 정찰도 배회도 하지 않는다. 덧붙여 `UWxPatrolComponent::GetNextIndex` 는 `NumPoints <= 1` 이면 모드와 무관하게 false 를 반환하는데(`WxPatrolComponent.cpp:44-48`), `ExecuteTask` 의 게이트는 `GetNumPoints() == 0` 뿐이라(`:40`) 포인트가 1개인 경로는 Loop/PingPong 로 지정해도 첫 도착에서 `bPatrolFinished` 가 걸려 같은 영구 점유에 빠진다.
- **제안**: `HandlePossessedPawnChanged` 시점에 커서를 되돌릴 훅이 없으므로, `ExecuteTask` 진입 시 현재 `Patrol` 의 포인트 수로 `PatrolCursor` 를 검증하고 범위를 벗어나면 0 으로 되돌리는 최소 방어가 실용적이다. 1포인트 경로는 `GetNumPoints() <= 1` 로 게이트를 넓혀 `Failed` 로 떨어뜨리면 하위 폴백(배회 등)이 정상 동작한다.
- **확신도**: 중간 — 컨트롤러가 폰과 1:1 로 생성·파괴되고 경로가 바뀌지 않는 운용이면 드러나지 않는다.

### 3. 🟢 Blackboard 가 없으면 `OnTargetChanged` 발행과 포커스 갱신까지 함께 막힌다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:239-243`
- **범주**: 설계/구조
- **문제**: `SetTargetActor` 는 Blackboard 가 없으면 조기 반환한다. 그런데 그 뒤에 있는 `AppliedTarget` 갱신, 타겟 소실 구독, `OnTargetChanged.Broadcast`, AI 포커스 설정은 Blackboard 와 아무 관련이 없다. BT 를 돌리지 않는 폰(= `AWxEnemyController::OnPossess` 가 `RunBehaviorTree` 를 부르지 않는 경로)에서는 감지 자체가 조용히 죽어, `AWxEnemyCharacter::SetHasAITarget` 같은 BB 무관 소비자까지 아무 통보를 받지 못한다.
- **제안**: Blackboard 가드를 `WxBlackboardKeys::SetTargetActor` 호출 한 줄로 좁힌다.
- **확신도**: 중간(의도된 설계일 수 있음) — 이 모듈이 BT 전제 운용만 지원한다는 판단이면 현행이 맞다.

### 4. 🟢 `UWxBTTask_Wander::TotalTime` 은 `Duration` 의 복사본일 뿐이다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h:70`, `Private/WxBTTask_Wander.cpp:50`, `:82`
- **범주**: 중복/복잡도
- **문제**: `TotalTime` 은 `ExecuteTask` 에서 `Duration` 을 그대로 받고 그 외에는 변하지 않는다. 비교(`:82`)도 `Duration` 을 직접 쓰면 되므로, 상태를 하나 더 들고 있을 근거가 없다.
- **제안**: `TotalTime` 을 지우고 `ElapsedTime >= Duration` 으로 비교한다.
- **확신도**: 높음

### 5. 🟢 사용하지 않는 모듈 의존 2건
- **위치**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs:19-20`
- **범주**: 중복/복잡도
- **문제**: `GameplayTasks`·`NavigationSystem` 을 참조하는 include 나 심볼이 모듈 안에 하나도 없다(정찰·복귀는 `UBTTask_MoveTo` 를 통해 `AIModule` 이 처리한다). 둘 다 `PublicDependencyModuleNames` 라 하위 참조 모듈까지 전파된다.
- **제안**: 제거한다. 필요해지면 `PrivateDependencyModuleNames` 로 되살린다.
- **확신도**: 높음

### 6. 🟢 공개 헤더가 `AbilitySystemComponent.h` 전체를 끌어오고, 같은 타입을 전방 선언까지 한다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h:6`, `:12`
- **범주**: 중복/복잡도
- **문제**: 헤더가 실제로 필요한 것은 `FGameplayAbilitySpecHandle` 과 `TWeakObjectPtr<UAbilitySystemComponent>` 뿐인데 ASC 헤더 전체를 공개 헤더에서 포함한다. 게다가 `:12` 에 `class UAbilitySystemComponent;` 전방 선언이 함께 있어 둘 중 하나는 의미가 없다.
- **제안**: 포함을 `GameplayAbilitySpecHandle.h`(또는 `GameplayAbilitySpec.h`) 로 좁히고 전방 선언을 유지한다. cpp 는 이미 `AbilitySystemComponent.h` 를 포함하고 있다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Private/WxBTTask_ActivateAbility.cpp`, `Private/WxBTComposite_RandomChoice.cpp`, `Private/WxBTTask_Patrol.cpp`, `Private/WxBTDecorator_BeyondLeash.cpp`, `Private/WxPatrolComponent.cpp`, `Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Private/WxBTTask_Wander.cpp`, `Private/WxBTTask_ReturnHome.cpp`, `Private/WxBTDecorator_AttributeRatio.cpp`, `Private/WxBTDecorator_RandomWeight.cpp`, `Private/WxBTService_TargetDistance.cpp`, `Private/WxAnimNotify_ReportNoise.cpp`, `Private/WxAIModule.cpp`, 대응 `Public/` 헤더 전부, `WxAI.Build.cs`, `WxAI.uplugin`, `README.md`. 경계 확인용으로 `Source/WxGame/Controller/WxEnemyController.cpp` 를 함께 읽었다.
- **검증한 규칙**: 소스 첫 줄 저작권(29/29 통과), 인라인 함수 정의(0건), 람다(0건), `BlueprintCallable`/`BlueprintPure`(0건), 델리게이트 콜백 `Handle` prefix(`HandleTargetPerceptionUpdated`·`HandleTargetDeathTagChanged`·`HandleTargetEndPlay`·`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded` 전부 준수), `Wx` prefix(전부 준수), override 의 `Super::` 호출(전부 준수), 플러그인 의존 DAG(`WxAI.Build.cs`·`WxAI.uplugin` 모두 `WxCore` 외 Wx 참조 없음 — 위반 없음).
- **엔진 대조로 확인해 발견에서 제외한 항목**: `UBTComposite_Selector` 의 `GetNextChildHandler` 는 5.8 에서 순수 가상 오버라이드로 디스패치되므로 `UWxBTComposite_RandomChoice` 의 재정의가 정상 동작한다. `FWxBTRandomChoiceMemory : FBTCompositeMemory` 배치는 엔진 `FBTParallelMemory` 와 동일 패턴이다. `NotifyDecoratorsOnFailedActivation` 을 사전 필터에서 직접 부르는 것은 `bUseDecoratorsFailedActivationCheck` 가 false 라 엔진 경로와 동치다. `UAIPerceptionComponent::ConfigureSense` 는 센스 클래스 기준 교체라 `PostInitProperties` 재호출이 중복을 만들지 않는다. `USplineComponent::GetLocationAtSplinePoint` 는 5.8 에서 인덱스를 클램프하므로 커서가 범위를 벗어나도 원점 이동은 발생하지 않는다. `UWxBTTask_ActivateAbility::AbortTask` 가 `CanBeCanceled()==false` 인 Override 그룹 어빌리티에서 즉시 끝나지 않는 것은 주석대로 의도된 지연 abort 이며, 자연 종료 시 `FinishLatentAbort` 로 마감된다.
- **미검토 / 한계**: BT/Blackboard 에셋과 BP 배선(키 등록 여부, `UWxBTDecorator_BeyondLeash` 의 `FlowAbortMode` 실제 설정, `MoveSpeedEffect`/`Attribute` 지정 여부)은 범위 밖이라 확인하지 않았다. 멀티플레이 4인 협동에서의 감지 부하·복제 동작은 정적 분석만 했고 실측하지 않았다. `WxBTComposite_RandomChoice` 의 추첨 분포는 코드 검토만 했고 통계적으로 검증하지 않았다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 29파일 — `/module-review`로 갱신*
