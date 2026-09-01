# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 엔진 BT/Perception 규약을 정확히 따르고, 위험한 지점(노드 메모리 레이아웃, 어빌리티 종료 통지의 재진입, 타겟 소실 구독 해제)마다 근거 주석이 남아 있어 의도가 읽힌다. 프로젝트 코딩·모듈 규칙 위반은 한 건도 없다. 이번 리뷰는 `Plugins/WxAI` 의 C++ 29파일(2,066줄)을 전부 읽고, 상태·수명주기가 얽힌 `WxAIPerceptionComponent`·`WxBTTask_ActivateAbility`·`WxBTComposite_RandomChoice`·`WxBTTask_Patrol` 은 UE 5.8 엔진 소스와 대조하며 깊게 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 빙의 해제(UnPossess) 경로에서 AI 포커스와 회전 모드가 원복되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:170-176`, `:246-249`, `:256-284`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged` 는 `SetTargetActor(nullptr)` 로 타겟을 되돌리지만, `SetTargetActor` 의 원복 구간(`AIC->ClearFocus`, 회전 모드 복원)은 `Cast<ACharacter>(AIC->GetPawn())` 가 유효할 때만 실행된다(`:256-261`). 엔진 `AController::UnPossess` 는 `OnUnPossess()` 에서 폰 참조를 끊은 **뒤에** `OnPossessedPawnChanged` 를 방송하므로(Controller.cpp:386-408), 이 델리게이트가 도착한 시점엔 `GetPawn()` 이 이미 null 이라 원복 구간이 통째로 건너뛰어진다. 결과는 두 가지다 — (a) 컨트롤러에 `EAIFocusPriority::Gameplay` 포커스가 옛 타겟을 가리킨 채 남고, (b) 이전 폰의 `bUseControllerDesiredRotation=true` / `bOrientRotationToMovement=false` 가 그대로 남는다. 포커스는 폰이 아니라 **컨트롤러** 소유라 다음 빙의까지 살아남는데, 그다음 `SetTargetActor(nullptr)` 은 `AppliedTarget` 이 이미 null 이라 `:246-249` 에서 조기 반환하므로 영영 정리되지 않는다. `AWxAIController::OnPossess` 의 "재사용된 폰도 새 빙의에서는 타겟 없이 시작한다"(`Source/WxGame/Controller/WxAIController.cpp:43`)가 폰 재사용 운용을 전제하므로 재현 가능한 경로이며, 재빙의된 AI 가 타겟 없이도 옛 타겟 쪽을 계속 바라보게 된다.
- **제안**: 포커스 해제는 폰과 무관하므로 `Character` 유효성 검사 앞으로 올린다. 회전 모드 복원은 `HandlePossessedPawnChanged` 가 받은 `OldPawn` 을 넘겨 그 폰의 무브먼트에 적용하도록 분리한다.
- **확신도**: 중간 — 컨트롤러가 폰과 항상 함께 파괴되는 운용이라면 무해하다.

### 2. 🟡 정찰 지점이 1개인 경로는 MoveMode 와 무관하게 영구 정지한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:40`, `:47-50`, `:99-112`, `Private/WxPatrolComponent.cpp:44-48`
- **범주**: 버그/정확성
- **문제**: `UWxPatrolComponent::GetNextIndex` 는 `NumPoints <= 1` 이면 MoveMode 를 보기도 전에 false 를 반환한다(`:44-48`). 그런데 `ExecuteTask` 의 게이트는 `GetNumPoints() == 0` 뿐이라(`:40`) 포인트가 1개인 경로도 통과한다. 그 결과 폰은 그 한 점까지 이동해 `Succeeded` 로 끝나고, `OnTaskFinished` 가 `GetNextIndex` 실패를 "Once 로 경로 끝에 도달"으로 해석해 `bPatrolFinished = true` 를 세운다(`:109-110`). `bPatrolFinished` 를 되돌리는 지점은 어디에도 없으므로 이후 `ExecuteTask` 는 매번 `:47-50` 에서 곧장 `InProgress` 를 반환해 브랜치를 영구 점유한다 — 정찰도, 뒤따르는 폴백(배회 등)도 더는 돌지 않는 정지 상태가 된다. 디자이너가 `Loop`/`PingPong` 을 골랐는데 런타임 동작은 "그 자리에 영원히 서 있기"로 조용히 바뀌는 셈이다.
- **제안**: `ExecuteTask` 게이트를 `GetNumPoints() <= 1` 로 넓혀 `Failed` 로 떨어뜨린다. 그러면 하위 폴백이 정상 동작하고, "한 지점을 지키게 하고 싶다"는 의도는 정찰이 아니라 별도 노드로 표현하게 된다.
- **확신도**: 중간 — 1포인트 경로를 저작할 일이 없다면 드러나지 않는다. (다중 포인트 `Once` 경로가 끝에서 브랜치를 점유하는 것은 헤더 주석대로 의도된 동작이다.)

### 3. 🟡 RandomChoice 사전 필터가 엔진 탐색이 닿지 않았을 자식까지 실패 통지한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:57-67` (특히 `:64-65`)
- **범주**: 버그/정확성
- **문제**: 엔진 `UBTCompositeNode::FindChildToExecute` 는 조건을 통과하는 첫 자식을 만나면 `break` 하므로, 그 **뒤쪽** 자식들에는 `NotifyDecoratorsOnFailedActivation` 이 절대 도달하지 않는다(BTCompositeNode.cpp:34-63). 반면 이 사전 필터는 전체 자식을 한 바퀴 돌며 조건 실패한 자식 **모두** 에 통지를 보낸다. 통지는 단순 알림이 아니라 부작용이 있다 — 해당 자식의 모든 데코레이터에 `WrappedOnNodeProcessed` 를 돌리고(`UBTDecorator_Loop` 는 여기서 반복 카운트를 깎고 부모에 `SetChildOverride` 를 건다), `LowerPriority`/`Both` 데코레이터를 관찰자(aux)로 등록한다(BTCompositeNode.cpp:297-315). 특히 `SetChildOverride` 가 걸리면 다음 진입에서 `GetNextChild` 가 `GetNextChildHandler` 를 아예 건너뛰고 그 자식을 강제 선택하므로(BTCompositeNode.cpp:603-607) 무작위 추첨 자체가 무력화된다. 코드 주석은 "사전 필터가 그 경로를 건너뛰므로 여기서 대신 보낸다"고 밝히지만, 엔진이 실제로 통지하는 범위는 *선택된 자식보다 앞쪽* 뿐이라는 점이 반영돼 있지 않다.
- **제안**: 추첨을 먼저 끝낸 뒤, 선택된 인덱스보다 앞쪽에서 조건 실패한 자식에 대해서만 `NotifyDecoratorsOnFailedActivation` 을 보내 엔진 Selector 와 통지 범위를 일치시킨다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 실제 피해는 자식에 어떤 데코레이터가 붙어 있느냐에 달렸고, BT 에셋 내용은 이번 리뷰 범위 밖이다. 조건 데코레이터만 쓰는 트리라면 관찰자 등록이 조금 늘어나는 정도로 끝난다.

### 4. 🟢 Blackboard 가 없으면 `OnTargetChanged` 발행·구독 해제·포커스 갱신까지 함께 막힌다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:239-243`
- **범주**: 설계/구조
- **문제**: `SetTargetActor` 는 Blackboard 가 없으면 조기 반환한다. 그런데 그 뒤에 있는 `AppliedTarget` 갱신, 타겟 소실 구독 해제, `OnTargetChanged.Broadcast`, AI 포커스 설정은 Blackboard 와 아무 관련이 없다. `AWxAIController::OnPossess` 는 폰이 BT 를 들고 있을 때만 `RunBehaviorTree` 를 부르므로(`Source/WxGame/Controller/WxAIController.cpp:37-40`) Blackboard 가 없는 AI 폰이 존재할 수 있고 — 헤더가 "적이든 소환수든" 모두 이 컨트롤러가 몬다고 밝힌다 — 그런 폰에서는 감지가 조용히 죽고 `AWxEnemyCharacter::SetHasAITarget` 같은 BB 무관 소비자도 아무 통보를 받지 못한다. 덤으로 죽은 타겟 구독이 해제되지 않은 채 남는다.
- **제안**: Blackboard 가드를 `WxBlackboardKeys::SetTargetActor` 호출 한 줄로 좁힌다.
- **확신도**: 중간(의도된 설계일 수 있음) — 이 모듈이 BT 전제 운용만 지원한다는 판단이면 현행이 맞다.

### 5. 🟢 `EWxTeam` 이 WxAI 에 있는데 WxAI 안에는 사용처가 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:9`
- **범주**: 설계/구조
- **문제**: `EWxTeam` 을 참조하는 코드는 전부 `WxGame`(`Source/WxGame/Character/WxCharacterBase.h:142` 등)이고, WxAI 자신은 이 enum 을 한 번도 쓰지 않는다(피아 판정은 엔진 `FGenericTeamId::GetAttitude` 로 한다). 팀 구분은 전투·타게팅·AI 가 함께 쓰는 공용 계약인데, 도메인 플러그인은 `WxCore` 외 다른 Wx 플러그인을 참조할 수 없으므로 다른 도메인은 이 enum 에 손을 댈 수 없다. 실제로 `WxCombat` 은 팀 정보를 엔진 `IGenericTeamAgentInterface` 로만 다루며(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:16`, `Private/Targeting/WxTargetingFilterTask_Team.cpp:11`) `EWxTeam` 의 의미(Player/Enemy/Neutral)를 알지 못한다.
- **제안**: `EWxTeam` 을 `WxCore` 로 옮긴다. 도메인 타입이 아니라 도메인 간 공용 계약이므로 WxCore 의 역할에 맞는다.
- **확신도**: 중간 — 지금 당장 깨지는 곳은 없고, 팀 의미를 영원히 `WxGame` 안에만 두겠다는 방침이면 오히려 `WxGame` 으로 내리는 편이 맞다. 어느 쪽이든 현 위치는 아니다.

### 6. 🟢 Wander 가 내비메시를 보지 않고 이동 입력을 넣는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:96`
- **범주**: 버그/정확성
- **문제**: `Pawn->AddMovementInput(MoveDirection, 1.f)` 를 `Duration` 동안 매 틱 그대로 밀어 넣을 뿐, 목적지가 내비메시 위인지 낭떠러지인지 확인하지 않는다. 같은 모듈의 정찰·복귀는 `UBTTask_MoveTo` 를 통해 내비게이션에 맡기는데 배회만 원시 입력을 쓴다. 오픈월드 지형에서 8방향 중 하나를 무작위로 골라 밀면 적이 절벽 아래나 내비메시 밖으로 걸어 나갈 수 있다.
- **제안**: 방향 선택 시 `UNavigationSystemV1::NavigationRaycast` 나 `ProjectPointToNavigation` 으로 목표 지점을 검증하고, 실패하면 다른 방향을 고르거나 `Failed` 로 마감한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 전투 중 짧은 스트레이프 용도로 한정하고 레벨에서 낙하를 막는다면 현행으로 충분하다. 실제 낙하 사례를 확인하지는 않았다.

### 7. 🟢 감속 GE 부여/제거 코드가 Patrol 과 Wander 에 그대로 중복돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58-67`·`:87-91` 과 `Private/WxBTTask_Wander.cpp:52-61`·`:104-108`, 선언은 `Public/WxBTTask_Patrol.h:37-48` 과 `Public/WxBTTask_Wander.h:54-65`
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 선언(주석 문구까지 동일), `MakeOutgoingSpec` → `SetSetByCallerMagnitude` → `ApplyGameplayEffectSpecToSelf` 적용부, `OnTaskFinished` 의 제거부가 두 클래스에 문자 그대로 같다. 모듈에서 가장 큰 중복이며, 앞으로 감속 정책(예: SetByCaller 태그 변경, 적용 실패 로깅)을 바꾸면 두 곳을 같이 고쳐야 한다.
- **제안**: 두 태스크가 공유하는 얇은 베이스(예: `UWxBTTask_SlowedMoveBase`)로 두 필드와 적용·제거를 올린다. 다만 프로젝트 관례상 소폭 중복은 용인하므로, 세 번째 사용처가 생길 때까지 미루는 판단도 타당하다.
- **확신도**: 높음(중복은 사실) / 조치 여부는 판단 사항

### 8. 🟢 사용하지 않는 모듈 의존 2건
- **위치**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs:19-20`
- **범주**: 중복/복잡도
- **문제**: `GameplayTasks`·`NavigationSystem` 을 참조하는 include 나 심볼이 모듈 안에 하나도 없다(정찰·복귀는 `UBTTask_MoveTo` 를 통해 `AIModule` 이 처리한다). 게다가 `AIModule.Build.cs` 가 이미 이 둘을 `PublicDependencyModuleNames` 로 재수출하므로 이 두 줄은 기능적으로 완전한 무의미다.
- **제안**: 제거한다. 직접 쓰게 되면 `PrivateDependencyModuleNames` 로 되살린다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Private/WxBTTask_ActivateAbility.cpp`, `Private/WxBTComposite_RandomChoice.cpp`, `Private/WxBTTask_Patrol.cpp`, `Private/WxBTDecorator_BeyondLeash.cpp`, `Private/WxPatrolComponent.cpp`, `Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Private/WxBTTask_Wander.cpp`, `Private/WxBTTask_ReturnHome.cpp`, `Private/WxBTDecorator_AttributeRatio.cpp`, `Private/WxBTDecorator_RandomWeight.cpp`, `Private/WxBTService_TargetDistance.cpp`, `Private/WxAnimNotify_ReportNoise.cpp`, `Private/WxAIModule.cpp`, 대응 `Public/` 헤더 전부, `WxAI.Build.cs`, `WxAI.uplugin`, `README.md`. 경계 확인용으로 `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCombat/.../WxCombatLibrary.cpp` 를 함께 읽었다.
- **검증한 규칙(위반 0건)**: 소스 첫 줄 저작권(29/29 통과), 인라인 함수 정의(0건), 람다(0건), `BlueprintCallable`/`BlueprintPure`(0건), 델리게이트 콜백 `Handle` prefix(`HandleTargetPerceptionUpdated`·`HandleTargetDeathTagChanged`·`HandleTargetEndPlay`·`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded` 전부 준수), `Wx` prefix(전부 준수), override 의 `Super::` 호출(호출이 유의미한 지점 전부 준수), 플러그인 의존 DAG(`WxAI.Build.cs`·`WxAI.uplugin` 모두 `WxCore` 외 Wx 참조 없음).
- **엔진 대조로 확인해 발견에서 제외한 항목**: `FWxBTRandomChoiceMemory : FBTCompositeMemory` 배치와 `GetInstanceMemorySize`/`InitializeMemory` 오버라이드는 엔진 `FBTParallelMemory` 패턴과 동일하며, `UBTCompositeNode::InitializeMemory` 가 base 서브오브젝트만 placement-new 하므로 Super 호출 뒤 `LastChosenChild` 를 쓰는 순서도 옳다. `NotifyDecoratorsOnFailedActivation` 직접 호출 자체는 `bUseDecoratorsFailedActivationCheck == false` 라 엔진 경로와 동치다(범위 문제는 발견 3 참조). `UWxBTTask_ActivateAbility` 의 `FScopedAbilityListLock` 은 5.8 `GiveAbility` 가 잠금 중 `AbilityPendingAdds` 로 우회하므로 루프 중 배열 재할당을 실제로 막는다. `UBehaviorTreeComponent::GetTaskStatus` 는 `FindTemplateNode`/`IsInstanced` 로 인스턴스 노드를 처리하므로 `bCreateNodeInstance` 태스크에서 `Aborting` 판정이 정상 동작한다. `EWxWanderDirection` 은 `UENUM` 에 `Bitflags` 메타가 없어도 `SPropertyEditorNumeric` 이 enum 값을 비트 인덱스로 쓰므로 `Directions` 비트마스크 편집이 정상이다. 정찰 커서(`PatrolCursor`·`bPatrolFinished`)가 빙의 교체를 넘어 남을 것으로 의심했으나, `bStopAILogicOnUnposses` 기본값 true 가 `CleanupBrainComponent → RemoveAllInstances` 로 노드 인스턴스를 파기하므로 사실이 아니다.
- **미검토 / 한계**: BT/Blackboard 에셋과 BP 배선(키 등록 여부, `UWxBTDecorator_BeyondLeash` 의 `FlowAbortMode` 실제 설정, 각 자식에 붙은 데코레이터 조합, `MoveSpeedEffect`/`Attribute` 지정 여부)은 범위 밖이라 확인하지 않았다 — 발견 3 의 실제 영향은 여기에 달려 있다. 멀티플레이 협동에서의 감지 부하·복제 동작은 정적 분석만 했고 실측하지 않았다. `WxBTComposite_RandomChoice` 의 추첨 분포는 코드 검토만 했고 통계적으로 검증하지 않았다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 29파일 — `/module-review`로 갱신*
