# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 엔진 BT/Perception 규약을 정확히 따르고, 위험한 지점(노드 메모리 레이아웃, 어빌리티 종료 통지의 재진입, 타겟 소실 구독 해제, 시야 기록 초기화)마다 근거 주석이 남아 있어 의도가 읽힌다. 프로젝트 코딩·모듈 규칙 위반은 한 건도 없다. 이번 리뷰는 `Plugins/WxAI` 의 C++ 29파일을 전부 읽고, 상태·수명주기가 얽힌 `WxAIPerceptionComponent`·`WxBTTask_ActivateAbility`·`WxBTComposite_RandomChoice`·`WxBTTask_Patrol`·`WxBTDecorator_BeyondLeash` 는 UE 5.8 엔진 소스와 대조하며 깊게 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 8 |

## 결과

### 1. 🟡 빙의 해제(UnPossess) 경로에서 AI 포커스와 회전 모드가 원복되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:169-175`, `:245-248`, `:255-283`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged` 는 `SetTargetActor(nullptr)` 로 타겟을 되돌리지만, `SetTargetActor` 의 원복 구간(`AIC->ClearFocus`, 회전 모드 복원)은 `Cast<ACharacter>(AIC->GetPawn())` 가 유효할 때만 실행된다(`:255-260`). 엔진 `AController::UnPossess` 는 `OnUnPossess()` 안에서 `SetPawn(nullptr)` 로 폰 참조를 끊은 **뒤에** `OnPossessedPawnChanged` 를 방송하므로(UE 5.8 `Controller.cpp`), 이 델리게이트가 도착한 시점엔 `GetPawn()` 이 이미 null 이라 원복 구간이 통째로 건너뛰어진다. 결과는 두 가지다 — (a) 컨트롤러에 `EAIFocusPriority::Gameplay` 포커스가 옛 타겟을 가리킨 채 남고, (b) 이전 폰의 `bUseControllerDesiredRotation=true` / `bOrientRotationToMovement=false` 가 그대로 남는다. 포커스는 폰이 아니라 **컨트롤러** 소유라 다음 빙의까지 살아남는데, 그다음 `SetTargetActor(nullptr)` 은 `AppliedTarget` 이 이미 null 이라 `:245-248` 에서 조기 반환하므로 영영 정리되지 않는다. `AWxAIController::OnPossess` 의 "재사용된 폰도 새 빙의에서는 타겟 없이 시작한다"(`Source/WxGame/Controller/WxAIController.cpp:43`)가 폰 재사용 운용을 전제하므로 재현 가능한 경로이며, 재빙의된 AI 가 타겟 없이도 옛 타겟 쪽을 계속 바라보게 된다.
- **제안**: 포커스 해제는 폰과 무관하므로 `Character` 유효성 검사 앞으로 올린다. 회전 모드 복원은 `HandlePossessedPawnChanged` 가 이미 받고 있는 `OldPawn` 을 넘겨 그 폰의 무브먼트에 적용하도록 분리한다.
- **확신도**: 중간 — 컨트롤러가 항상 폰과 함께 파괴되는 운용(현재 `AWxSpawner::Respawn` 경로)이라면 무해하다.

### 2. 🟡 정찰 지점이 1개인 경로는 MoveMode 와 무관하게 영구 정지한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:40`, `:47-50`, `:99-112`, `Private/WxPatrolComponent.cpp:44-48`
- **범주**: 버그/정확성
- **문제**: `UWxPatrolComponent::GetNextIndex` 는 `NumPoints <= 1` 이면 MoveMode 를 보기도 전에 false 를 반환한다(`:44-48`). 그런데 `ExecuteTask` 의 게이트는 `GetNumPoints() == 0` 뿐이라(`:40`) 포인트가 1개인 경로도 통과한다. 그 결과 폰은 그 한 점까지 이동해 `Succeeded` 로 끝나고, `OnTaskFinished` 가 `GetNextIndex` 실패를 "Once 로 경로 끝에 도달"으로 해석해 `bPatrolFinished = true` 를 세운다(`:109-110`). `bPatrolFinished` 를 되돌리는 지점은 어디에도 없으므로 이후 `ExecuteTask` 는 매번 `:47-50` 에서 곧장 `InProgress` 를 반환해 브랜치를 영구 점유한다 — 정찰도, 뒤따르는 폴백(배회 등)도 더는 돌지 않는 정지 상태가 된다. 디자이너가 `Loop`/`PingPong` 을 골랐는데 런타임 동작은 "그 자리에 영원히 서 있기"로 조용히 바뀌는 셈이다.
- **제안**: `ExecuteTask` 게이트를 `GetNumPoints() <= 1` 로 넓혀 `Failed` 로 떨어뜨린다. 그러면 하위 폴백이 정상 동작하고, "한 지점을 지키게 하고 싶다"는 의도는 정찰이 아니라 별도 노드로 표현하게 된다.
- **확신도**: 중간 — 1포인트 경로를 저작할 일이 없다면 드러나지 않는다. (다중 포인트 `Once` 경로가 끝에서 브랜치를 점유하는 것은 헤더 주석대로 의도된 동작이다.)

### 3. 🟡 `UWxBTTask_ActivateAbility` 의 `OnTaskFinished` 오버라이드는 호출되지 않는 죽은 코드다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:9-12`(생성자), `:165-170`
- **범주**: 버그/정확성
- **문제**: 엔진은 `UBTTaskNode::bNotifyTaskFinished` 가 true 일 때만 `OnTaskFinished` 를 부른다(UE 5.8 `BTTaskNode.cpp` 의 `WrappedOnTaskFinished`). 이 플래그는 `UBTTaskNode` 생성자에서 false 로 초기화되고 `UPROPERTY` 도 아니어서 에디터로 켤 수 없으며, 켜는 방법은 생성자에서 직접 대입하거나 `INIT_TASK_NODE_NOTIFY_FLAGS()` 를 호출하는 것뿐이다. 그런데 이 태스크의 생성자는 `bCreateNodeInstance` 만 세운다 — 같은 모듈의 `UWxBTTask_Patrol`(`WxBTTask_Patrol.cpp:19`)·`UWxBTTask_Wander`(`WxBTTask_Wander.cpp:16`)는 둘 다 명시적으로 켜고 있어 누락으로 읽힌다. 따라서 종료 시 구독 해제를 보장하려고 둔 `CleanUp()` 안전망이 실제로는 한 번도 돌지 않는다. 현재는 `ExecuteTask`(`:69`, `:89`)·`AbortTask`(`:103`, `:116`)·`HandleAbilityEnded`(`:150`)가 모든 종료 경로에서 각자 `CleanUp()` 을 부르고 있어 실피해는 잠복 상태지만, "종료하면 반드시 정리된다"는 불변식이 조용히 깨져 있어 앞으로 이 태스크를 손대는 쪽이 오독하기 쉽다.
- **제안**: 생성자에 `INIT_TASK_NODE_NOTIFY_FLAGS()` 를 추가한다(오버라이드 감지로 `bNotifyTaskFinished` 가 자동으로 켜진다). 안전망을 쓰지 않을 방침이면 반대로 `OnTaskFinished` 오버라이드를 지운다.
- **확신도**: 높음(플래그가 꺼져 있다는 사실) / 실피해는 현재 없음

### 4. 🟢 Blackboard 가 없으면 `OnTargetChanged` 발행·구독 해제·포커스 갱신까지 함께 막힌다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:238-242`
- **범주**: 설계/구조
- **문제**: `SetTargetActor` 는 Blackboard 가 없으면 조기 반환한다. 그런데 그 뒤에 있는 `AppliedTarget` 갱신, 타겟 소실 구독 해제, `OnTargetChanged.Broadcast`, AI 포커스 설정은 Blackboard 와 아무 관련이 없다. `AWxAIController::OnPossess` 는 폰이 BT 를 들고 있을 때만 `RunBehaviorTree` 를 부르므로(`Source/WxGame/Controller/WxAIController.cpp:37-40`) Blackboard 가 없는 AI 폰이 존재할 수 있고, 그런 폰에서는 감지가 조용히 죽고 `AWxEnemyCharacter::SetHasAITarget` 같은 BB 무관 소비자도 아무 통보를 받지 못한다. 덤으로 `AppliedTarget` 이 옛 타겟을 가리킨 채 남아 `HandleTargetPerceptionUpdated`(`:98`)의 "타겟 없을 때만 획득" 게이트가 영구히 닫힌다.
- **제안**: Blackboard 가드를 `WxBlackboardKeys::SetTargetActor` 호출 한 줄로 좁힌다.
- **확신도**: 중간(의도된 설계일 수 있음) — 이 모듈이 BT 전제 운용만 지원한다는 판단이면 현행이 맞다.

### 5. 🟢 피격 가해자 "즉시" 타게팅은 기존 타겟이 있으면 동작하지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:98`, 선언부 주석 `Public/WxAIPerceptionComponent.h:66-70`
- **범주**: 버그/정확성
- **문제**: 헤더는 Damage 센스의 목적을 "가해자를 즉시 `TargetActor` 로 인지하게 한다"로 적었지만, 모든 자극은 `HandleTargetPerceptionUpdated` 의 `!AppliedTarget.ResolveObjectPtr()` 게이트를 통과해야 한다. 즉 이미 A 를 쫓는 중이면 뒤에서 B 가 때려도 타겟이 바뀌지 않는다. 촉각을 시각·청각과 동일한 "타겟 없을 때만" 경로로 흘려보내면, 촉각을 따로 구현한 이유(가해자 우선 반응)가 사라진다.
- **제안**: 의도가 "가해자 우선"이라면 `HandlePawnHit` 에서 게이트를 우회해 `SetTargetActor(DamageInstigator)` 를 직접 부르거나, `FAIStimulus` 의 센스 종류가 Damage 인 자극만 기존 타겟을 덮어쓰도록 예외를 둔다. 반대로 현행이 의도라면 헤더 주석의 "즉시"를 고친다.
- **확신도**: 중간 — 어그로 고착이 의도된 전투 설계일 수 있다. 다만 코드와 주석 중 한쪽은 틀렸다.

### 6. 🟢 `EWxTeam` 이 WxAI 에 있는데 WxAI 안에는 사용처가 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:9`
- **범주**: 설계/구조
- **문제**: `EWxTeam` 을 참조하는 코드는 전부 `WxGame`(`Source/WxGame/Character/WxCharacterBase.h:142` 등)이고, WxAI 자신은 이 enum 을 한 번도 쓰지 않는다(피아 판정은 엔진 `FGenericTeamId::GetAttitude` 로 한다). 팀 구분은 전투·타게팅·AI 가 함께 쓰는 공용 계약인데, 도메인 플러그인은 `WxCore` 외 다른 Wx 플러그인을 참조할 수 없으므로 다른 도메인은 이 enum 에 손을 댈 수 없다.
- **제안**: `EWxTeam` 을 `WxCore` 로 옮긴다. 도메인 타입이 아니라 도메인 간 공용 계약이므로 WxCore 의 역할에 맞는다.
- **확신도**: 중간 — 지금 당장 깨지는 곳은 없고, 팀 의미를 영원히 `WxGame` 안에만 두겠다는 방침이면 오히려 `WxGame` 으로 내리는 편이 맞다. 어느 쪽이든 현 위치는 아니다.

### 7. 🟢 어빌리티 발동 루프에서 `ActivationResult` 가 후보 사이에 초기화되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:43`, `:51-63`, `:85-87`
- **범주**: 버그/정확성
- **문제**: `ActivationResult` 는 루프 진입 **전에** 한 번만 `InProgress` 로 초기화된다(`:43`). 루프는 후보마다 `ActivatedHandle` 을 먼저 세우고 `TryActivateAbility` 를 시도하며, 실패하면 `ActivatedHandle` 만 되돌린다(`:61`). 그런데 실패한 후보의 시도 중에 그 핸들로 `OnAbilityEnded` 가 도착하면 `HandleAbilityEnded` 가 `bIsActivating` 분기에서 `ActivationResult` 에 결과를 남기고(`:136-139`), 그 값은 지워지지 않는다. 이후 다른 후보가 발동에 성공했는데 발동 구간 안에서 스펙이 사라져 통지가 오지 않는 경우, `:85-87` 은 "통지 없이 비활성"으로 보고 실패 처리해야 할 상황을 **앞 후보의 잔여 결과**로 마감한다.
- **제안**: `ActivationResult = EBTNodeResult::InProgress;` 를 루프 안, 각 `TryActivateAbility` 직전으로 옮긴다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 재현 조건(발동 실패 + 그 시도 중 종료 통지 + 후속 후보의 통지 누락)이 겹겹이라 실측하지 못했다. 다만 한 줄 이동으로 없앨 수 있는 상태 오염이다.

### 8. 🟢 RandomChoice 사전 필터의 실패 통지 범위가 엔진보다 넓다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:57-67`(특히 `:64-65`)
- **범주**: 설계/구조
- **문제**: 엔진 `UBTCompositeNode::FindChildToExecute` 는 조건을 통과하는 첫 자식에서 `break` 하므로, 그 **뒤쪽** 자식들에는 `NotifyDecoratorsOnFailedActivation` 이 절대 도달하지 않는다(UE 5.8 `BTCompositeNode.cpp:35-67`). 반면 이 사전 필터는 전체 자식을 한 바퀴 돌며 조건 실패한 자식 **모두** 에 통지를 보내고, 통지는 `LowerPriority`/`Both` 데코레이터를 관찰자(aux)로 등록한다. 즉 엔진 Selector 라면 관찰자가 되지 않았을 "뒤쪽" 자식의 데코레이터가 등록되고, 그 조건이 뒤집히는 순간 `RequestExecution` 이 나가 지금 돌고 있는 추첨 결과를 끊고 재탐색·재추첨을 유발할 수 있다. (앞선 리뷰가 이 통지로 `UBTDecorator_Loop` 의 `SetChildOverride` 가 걸린다고 적었으나, 엔진 확인 결과 Loop 의 오버라이드는 `OnNodeActivation` 경로라 여기서는 걸리지 않는다. `WrappedOnNodeProcessed` 로 부작용이 있는 스톡 데코레이터는 `UBTDecorator_ForceSuccess` 뿐이고, 이 코드는 결과를 버리는 지역 변수를 넘기므로 무해하다.)
- **제안**: 그대로 두어도 무방하다. 무작위 Composite 는 자식 간 우선순위가 없으므로 "탈락한 모든 자식을 관찰"이 오히려 의미에 맞는다는 해석도 가능하다. 다만 이 비대칭을 헤더 주석에 한 줄로 남겨 두면 다음 사람이 다시 조사하지 않는다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 실제 영향은 자식에 붙은 데코레이터 조합(BT 에셋)에 달렸고 그것은 이번 범위 밖이다.

### 9. 🟢 Wander 에서 허용 방향을 전부 끄면 오히려 전방향 무작위가 된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:39-47`(특히 `:44-47`)
- **범주**: 버그/정확성
- **문제**: `Directions` 비트마스크에서 허용 방향을 하나도 고르지 않으면 `AllowedIndices` 가 비고, 그때 폴백이 `FMath::FRandRange(0.f, 360.f)` 로 **아무 방향**을 고른다. 디자이너가 체크를 모두 해제한 의도는 "이 방향들로는 움직이지 마라"인데 결과는 8방향 제약이 사라진 완전 자유 이동이라 의미가 정반대로 뒤집힌다.
- **제안**: `AllowedIndices` 가 비면 `EBTNodeResult::Failed` 로 마감해 상위 폴백이 돌게 한다.
- **확신도**: 중간 — 기본값이 `0xFF` 라 실수로 전부 끄는 경우가 흔하지는 않다.

### 10. 🟢 Wander 가 내비메시를 보지 않고 이동 입력을 넣는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:96`
- **범주**: 성능/안전
- **문제**: `Pawn->AddMovementInput(MoveDirection, 1.f)` 를 `Duration` 동안 매 틱 그대로 밀어 넣을 뿐, 목적지가 내비메시 위인지 낭떠러지인지 확인하지 않는다. 같은 모듈의 정찰·복귀는 `UBTTask_MoveTo` 를 통해 내비게이션에 맡기는데 배회만 원시 입력을 쓴다. 오픈월드 지형에서 8방향 중 하나를 무작위로 골라 밀면 적이 절벽 아래나 내비메시 밖으로 걸어 나갈 수 있다.
- **제안**: 방향 선택 시 `UNavigationSystemV1::NavigationRaycast` 나 `ProjectPointToNavigation` 으로 목표 지점을 검증하고, 실패하면 다른 방향을 고르거나 `Failed` 로 마감한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 전투 중 짧은 스트레이프 용도로 한정하고 레벨에서 낙하를 막는다면 현행으로 충분하다. 실제 낙하 사례를 확인하지는 않았다.

### 11. 🟢 감속 GE 부여/제거 코드가 Patrol 과 Wander 에 그대로 중복돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58-67`·`:87-91` 과 `Private/WxBTTask_Wander.cpp:52-61`·`:104-108`, 선언은 `Public/WxBTTask_Patrol.h:37-48` 과 `Public/WxBTTask_Wander.h:54-65`
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 선언(주석 문구까지 동일), `MakeOutgoingSpec` → `SetSetByCallerMagnitude` → `ApplyGameplayEffectSpecToSelf` 적용부, `OnTaskFinished` 의 제거부가 두 클래스에 문자 그대로 같다. 모듈에서 가장 큰 중복이며, 앞으로 감속 정책(예: SetByCaller 태그 변경, 적용 실패 로깅)을 바꾸면 두 곳을 같이 고쳐야 한다.
- **제안**: 두 태스크가 공유하는 얇은 베이스(예: `UWxBTTask_SlowedMoveBase`)로 두 필드와 적용·제거를 올린다. 다만 프로젝트 관례상 소폭 중복은 용인하므로, 세 번째 사용처가 생길 때까지 미루는 판단도 타당하다.
- **확신도**: 높음(중복은 사실) / 조치 여부는 판단 사항

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Private/WxBTTask_ActivateAbility.cpp`, `Private/WxBTComposite_RandomChoice.cpp`, `Private/WxBTTask_Patrol.cpp`, `Private/WxBTDecorator_BeyondLeash.cpp`, `Private/WxPatrolComponent.cpp`, `Private/WxBlackboardKeys.cpp`, `Private/WxBTTask_Wander.cpp`
- **훑은 파일**: `Private/WxBTTask_ReturnHome.cpp`, `Private/WxBTDecorator_AttributeRatio.cpp`, `Private/WxBTDecorator_RandomWeight.cpp`, `Private/WxBTService_TargetDistance.cpp`, `Private/WxAnimNotify_ReportNoise.cpp`, `Private/WxAIModule.cpp`, 대응 `Public/` 헤더 전부, `WxAI.Build.cs`, `WxAI.uplugin`, `README.md`. 경계 확인용으로 `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp` 를 함께 읽었다.
- **검증한 규칙(위반 0건)**: 소스 첫 줄 저작권(29/29 통과), 인라인 함수 정의(0건), 람다(0건), `BlueprintCallable`/`BlueprintPure`(0건), 델리게이트 콜백 `Handle` prefix(`HandleTargetPerceptionUpdated`·`HandleTargetDeathTagChanged`·`HandleTargetEndPlay`·`HandlePossessedPawnChanged`·`HandlePawnHit`·`HandleAbilityEnded` 전부 준수), `Wx` prefix(전부 준수), override 의 `Super::` 호출(호출이 유의미한 지점 전부 준수), 플러그인 의존 DAG(`WxAI.Build.cs`·`WxAI.uplugin` 모두 `WxCore` 외 Wx 참조 없음).
- **엔진 대조로 확인해 발견에서 제외한 항목**: `FWxBTRandomChoiceMemory : FBTCompositeMemory` 배치와 `GetInstanceMemorySize`/`InitializeMemory` 오버라이드는 엔진 `FBTParallelMemory` 패턴과 동일하며, `UBTCompositeNode::InitializeMemory` 가 base 서브오브젝트만 placement-new 하므로 Super 호출 뒤 `LastChosenChild` 를 쓰는 순서도 옳다. UE 5.8 `UBTComposite_Selector` 는 `OnNextChild` 델리게이트를 쓰지 않고 `GetNextChildHandler` 를 직접 오버라이드하므로 파생 클래스의 오버라이드가 정상 호출된다. `PostInitProperties` 에서의 `ConfigureSense` 3회 호출은 엔진이 같은 클래스 설정을 교체(중복 추가 없음)하고 아키타입 복사가 그보다 먼저 끝나므로 센스 중복 등록이 없다. `UWxBTTask_ReturnHome` 의 `ForgetTargetActor` → `ForgetActor` 는 `UAISense_Sight::OnListenerForgetsActor` 가 시야 쿼리의 `LastResult` 를 리셋하므로, 타겟이 계속 보이는 상태에서도 다음 갱신에 성공 자극이 다시 발행돼 재획득이 가능하다(주석의 주장이 사실). `UWxAIPerceptionComponent::HandlePawnHit` 의 수동 팀 필터는 `UAISense_Damage::Update` 가 피아 판정 없이 자극을 등록하므로 실제로 필요하다. `UWxBTDecorator_BeyondLeash` 의 `IsExecutingBranch` 조기 true 와 `RequestExecution(this)` 는 `EBTFlowAbortMode::LowerPriority` 관찰자 규약과 일치한다. `UWxBTTask_ActivateAbility` 의 `FScopedAbilityListLock` 과 `GetTaskStatus`/`FinishLatentTask` 의 노드 인스턴스 처리도 5.8 구현상 안전하다. 앞선 리뷰가 지적한 `WxAI.Build.cs` 의 미사용 의존(`GameplayTasks`·`NavigationSystem`)은 커밋 `6d849f7e` 에서 이미 제거돼 발견에서 뺐다.
- **미검토 / 한계**: BT/Blackboard 에셋과 BP 배선(키 등록 여부, `UWxBTDecorator_BeyondLeash` 의 `FlowAbortMode` 실제 설정, 각 자식에 붙은 데코레이터 조합, `MoveSpeedEffect`/`Attribute` 지정 여부)은 범위 밖이라 확인하지 않았다 — 발견 8 의 실제 영향은 여기에 달려 있다. 멀티플레이 협동에서의 감지 부하·복제 동작은 정적 분석만 했고 실측하지 않았다. `WxBTComposite_RandomChoice` 의 추첨 분포와 발견 7·10 의 재현은 코드 검토만 했고 런타임으로 확인하지 않았다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 29파일 — `/module-review`로 갱신*
