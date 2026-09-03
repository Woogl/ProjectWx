# WxAI — 코드 리뷰

> 28개 파일 약 2,100줄의 소형 모듈이고 상태가 매우 양호하다. 델리게이트 해제 대칭성, 노드 인스턴스/노드 메모리 상태 격리, 엔진 API 계약(`NavigationRaycast` 반환 의미, `ForgetActor` 가 Sight 쿼리 결과까지 리셋한다는 점, `FScopedAbilityListLock` 하에서의 어빌리티 목록 순회, `FBTCompositeMemory` 파생 패턴)이 모두 정확하며, 프로젝트 코딩·모듈 규칙 위반은 한 건도 없다. 이번 리뷰는 퍼셉션 컴포넌트와 BT 노드 cpp 전부를 읽고, 의심스러운 지점(빙의 전환, 어빌리티 동기 종료, 추첨 메모리, MoveTo 우회 경로)은 UE 5.8 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 퍼셉션이 폰의 회전 모드와 컨트롤러 포커스를 소유해, 해제 책임이 여러 경로로 흩어진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:266`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:276`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:279`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:81`
- **범주**: 설계/구조
- **문제**: 프로젝트 방침은 "AI 행동 결정은 BT 노드(판정=Decorator, 실행=Task), 퍼셉션은 감지·인식만" 이다. 그런데 `SetTargetActor` 는 타겟 동기화에 더해 `AIC->SetFocus`/`ClearFocus`(`:266~273`)와 폰 CMC 의 `bOrientRotationToMovement`/`bUseControllerDesiredRotation`(`:288~300`)까지 직접 쓴다. 즉 자극 처리 컴포넌트가 폰 이동 연출 상태를 소유한다. `UWxBTTask_Wander.h:27` 이 "회전 모드는 이 태스크가 아니라 퍼셉션이 발행한다"고 명시하고 README 도 퍼셉션의 담당으로 적어 둔 만큼 의도된 선택이지만, 대가가 코드에 그대로 드러난다 — 상태 원복이 `SetTargetActor(nullptr)`, `HandlePossessedPawnChanged(OldPawn)`(`:172`) 두 곳으로 갈라져 있고, 세 번째 해제 경로인 `EndPlay`(`:81`)는 구독만 끊고 회전 모드는 되돌리지 않는다.
  `EndPlay` 누락의 실제 노출 조건은 "폰이 살아남은 채 컨트롤러만 파괴" 다. 엔진은 `UWorld::DestroyActor` 에서 `EndPlay`(→ 컴포넌트 EndPlay, 여기서 `OnPossessedPawnChanged` 구독을 끊는다) 를 먼저 돌리고 그 뒤에 `AController::Destroyed`→`UnPossess` 를 부르므로, 이 경로에서는 `HandlePossessedPawnChanged` 가 아예 오지 않아 폰이 strafe 로 굳는다. 다만 현재 코드베이스는 폰·컨트롤러가 `AutoPossessAI` 로 1:1 생성·소멸(`Source/WxGame/Character/WxEnemyCharacter.cpp:23`, `Source/WxGame/Character/WxMinion.cpp:10`)되고 폰을 다른 컨트롤러에 재빙의하는 경로가 없어, 오늘 당장 재현되지는 않는다.
- **제안**: 장기적으로는 포커스·회전 모드 전환을 전투 브랜치를 여는 BT Task 쪽으로 옮기고 퍼셉션은 `OnTargetChanged` 발행까지만 맡는다(그러면 원복 지점이 태스크 종료 한 곳으로 모인다). 당장 손대지 않을 거라면 `EndPlay` 에서도 `SetStrafeRotation(GetOwnerPawn(), false)` 를 한 줄 추가해 세 경로를 대칭으로 맞춘다. 아키타입 기본값으로 되돌리는 현행 복원 방식 자체는 유지한다(진입 시점 값 저장/복원은 프로젝트가 이미 폐기한 방식이다).
- **확신도**: 낮음(의도된 설계일 수 있음)

### 2. 🟡 Patrol·Wander 의 감속 GE 부여/제거가 통째로 중복된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:87`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:79`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:131`
- **범주**: 중복/복잡도
- **문제**: 두 태스크가 `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 쌍(`WxBTTask_Patrol.h:39`·`:49`, `WxBTTask_Wander.h:63`·`:72`), `MakeOutgoingSpec` → `SetSetByCallerMagnitude(SetByCaller_MoveSpeedScale)` → `ApplyGameplayEffectSpecToSelf` 부여 블록, `OnTaskFinished` 의 제거 블록, 심지어 `GetStaticDescription` 의 "감속 GE 미지정" 분기까지 사실상 문자 단위로 같은 코드를 각자 들고 있다. 한쪽에만 적용한 수정(SetByCaller 태그 변경, 스택 처리, 핸들 유효성 강화)이 다른 쪽에 전파되지 않는다.
- **제안**: 베이스가 서로 달라(`UBTTask_MoveTo` vs `UBTTaskNode`) 중간 클래스는 부적절하다. 부여/제거 두 조각만 공유 헤더의 자유 함수로 빼는 정도가 침습이 적다. 반복이 2회에 그치는 동안은 현행 유지도 유효한 선택이다.
- **확신도**: 중간

### 3. 🟢 `bAvoidRepeat` 의 `LastChosenChild` 가 엔진의 탐색 재개 경로에서 갱신되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:125`
- **범주**: 버그/정확성
- **문제**: `LastChosenChild` 는 `GetNextChildHandler` 안에서만 기록된다. 그런데 UE 5.8 `UBTCompositeNode::GetNextChild` 는 `SearchData.SearchStart` 로 특정 노드에서 탐색을 재개하는 경우와 `OverrideChild` 가 걸린 경우 핸들러를 거치지 않고 자식을 직접 고른다. 그 경로로 실행된 자식은 기록에 남지 않아, 다음 추첨의 회피 기준이 실제 직전 자식과 어긋난다. 회피가 한 번 헛도는 정도라 영향은 작다.
- **제안**: 회피 정확도가 중요해지면 `NotifyChildExecution` 오버라이드에서 실제 실행된 자식 인덱스를 기록한다 — 세 경로가 모두 지나가는 지점이다. 그 전까지는 고칠 값어치가 크지 않다.
- **확신도**: 중간

### 4. 🟢 Wander: 이동 거리가 0이면 내비 검증이 통째로 무력해진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:49`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:61`
- **범주**: 버그/정확성
- **문제**: `:40` 의 주석이 "걸어갈 거리를 구할 수 없으면 길이 0 레이가 막힘으로 오지 않아 검증이 통째로 무력해진다" 고 경고하지만, 실제 가드는 `Movement` 널 여부만 본다(`:41~45`). `TravelDistance = GetMaxSpeed() * ... * Duration` 이 0이 되는 다른 경로 — SPD 어트리뷰트를 0으로 만드는 속박/둔화 GE 로 `MaxWalkSpeed` 가 0인 상태, `MovementMode` 가 `None` 인 상태, 디자이너가 `Duration` 을 0으로 저작한 경우 — 에서는 `NavigationRaycast(Pawn, NavStart, NavStart)` 가 길이 0 레이라 막힘으로 판정되지 않아 8방향 전부가 무검증 통과한다. 그 상태에선 어차피 이동이 거의 없어 실피해는 작지만, 클래스 주석이 선언한 계약과는 어긋난다.
- **제안**: `TravelDistance` 가 유의미한 값인지(`> KINDA_SMALL_NUMBER`) 확인해 아니면 방향 탐색 전에 `Failed` 로 마감한다.
- **확신도**: 중간

### 5. 🟢 `SelfActor` 는 엔진이 이름·동기화를 이미 소유하는 예약 키다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:20`, `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:33`, `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:11`
- **범주**: 중복/복잡도
- **문제**: UE 5.8 엔진의 `FBlackboard::KeySelf` 가 정확히 `TEXT("SelfActor")` 이고, `AAIController::SetPawn` 이 빙의·해제마다 그 키를 현재 폰으로 덮어쓴다. 즉 이 모듈은 엔진 예약 키 이름을 재선언하고 setter 까지 노출하고 있으며, 헤더 주석의 "SelfActor 의 SET/CLEAR 는 AIController 담당" 이라는 소유권 서술도 절반만 맞다. 실제로 유일하게 필요한 수동 개입은 "최초 빙의에서 Blackboard 가 `RunBehaviorTree` 이후에 생기는 바람에 엔진의 `SetPawn` 이 빈손으로 지나가는 순간" 뿐이고, `AWxAIController::OnUnPossess` 의 `SetSelfActor(BB, nullptr)`(`Source/WxGame/Controller/WxAIController.cpp:80`)는 곧이어 `Super::OnUnPossess` → `SetPawn(nullptr)` 이 같은 일을 하므로 순수 중복이다.
- **제안**: 상수는 `FBlackboard::KeySelf` 를 재사용하고, `SetSelfActor` 는 "BB 생성 직후 1회 보정" 이라는 유일한 용도를 헤더에 못박거나(그리고 unpossess 쪽 호출을 제거) 접근자 자체를 없앤다. `GetSelfActor` 는 타입 안전 조회로서 그대로 둘 값어치가 있다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 전체, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 소비자 쪽 `Source/WxGame/Controller/WxAIController.cpp`
- **교차 확인(엔진 소스 대조로 문제 없음을 확인한 항목)**:
  - 모듈 경계: Build.cs·uplugin 모두 `WxCore` 외 Wx 의존 없음. 코딩 규칙 6종(`Wx` prefix, 저작권 첫 줄, 람다, `Handle` prefix, `BlueprintCallable`, 인라인 정의) 전수 grep 결과 위반 0건.
  - `PostInitProperties` 의 `ConfigureSense` 3회 호출: `UAIPerceptionComponent::ConfigureSense` 가 센스 클래스 단위로 dedupe 하므로 아키타입 복사와 겹쳐도 중복 등록되지 않는다.
  - 생성자에서의 `OnTargetPerceptionUpdated.AddDynamic(this, ...)`: 네이티브 클래스 프로퍼티는 `CPF_Config` 가 아니면 `PostConstructLink` 에 오르지 않아 CDO 값으로 덮이지 않는다.
  - `ForgetTargetActor` 후 재감지: `UAIPerceptionComponent::ForgetActor` → `OnListenerForgetsActor` → `UAISense_Sight::ForgetPreviousResult` 로 시야 쿼리의 이전 결과가 리셋되므로, 계속 시야에 머문 타겟도 다음 갱신에서 새 자극이 온다(클래스 주석의 주장과 일치).
  - `UWxBTTask_Patrol` 이 `bPatrolFinished` 에서 `Super::ExecuteTask` 없이 `InProgress` 를 반환하는 경로: 노드 메모리가 0 초기화라 `UBTTask_MoveTo::AbortTask` 의 `MoveRequestID` 분기와 `OnTaskFinished` 의 옵저버 해제가 모두 무해하며, `StopTree`(사망 시 `StopLogic`)도 활성 태스크에 `WrappedAbortTask` 를 보내므로 트리가 영구 정지하지 않는다.
  - `UWxBTTask_ActivateAbility`: `FScopedAbilityListLock` 하에서 `GiveAbility` 가 `AbilityPendingAdds` 로 지연되므로 순회 중 배열 재할당이 없고, 브로드캐스트 도중의 `OnAbilityEnded.Remove`·`OnEndPlay.RemoveDynamic`·`UnregisterGameplayTagEvent` 는 UE 멀티캐스트가 공식 지원하는 사용이다.
  - `UWxBTDecorator_BeyondLeash` 의 매 프레임 폴링 → `RequestExecution(this)`: `EvaluateBranch` 가 `LowerPriority` 모드에서 "브랜치 진입 시도" 로 처리하므로 리시 이탈 시 하위 브랜치가 정상적으로 끊긴다. `GetLocationAtSplinePoint` 는 인덱스를 클램프하므로 `PatrolCursor` 가 앞서 나가도 널/원점 이동이 발생하지 않는다.
  - `EWxWanderDirection` 의 `Bitmask`/`BitmaskEnum` 메타: 에디터가 `UseEnumValuesAsMaskValuesInEditor` 기본값(false)에서 `1 << EnumValue` 로 해석하므로 `UENUM` 에 `Bitflags` 를 달지 않아도 의도대로 동작한다.
  - 데드 코드 없음: `WxBlackboardKeys` 접근자·`OnTargetChanged`·`ForgetTargetActor`·`GetMoveMode` 모두 실제 호출부가 있다.
- **미검토 / 한계**: BT·Blackboard·스플라인 에셋의 실제 구성은 범위 밖이라, `UWxBTComposite_RandomChoice` 헤더가 스스로 경고한 "LowerPriority·Both 데코가 붙은 뒤쪽 자식이 추첨 결과를 끊는" 상황이 현재 트리에서 실제로 일어나는지, `UWxBTDecorator_BeyondLeash` 가 리시 브랜치보다 낮은 우선순위에 배치돼 엔진의 우선순위 ensure 에 걸리는 트리가 있는지는 확인하지 않았다. `UWxPatrolComponent::ConfigureSpline` 이 `OnRegister` 에서 트랜잭션 없이 스플라인 포인트 타입을 덮어쓰는 것이 에디터 Undo 와 어떻게 상호작용하는지도 실행 검증하지 않았다. 멀티플레이는 BT·퍼셉션이 서버 전용이라는 전제만 확인했고 실제 복제 동작은 검증하지 않았다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 28파일 — `/module-review`로 갱신*
