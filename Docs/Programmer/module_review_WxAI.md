# WxAI — 코드 리뷰

> 30개 파일 약 2,200줄의 소형 모듈이고 상태가 매우 양호하다. 직전 리뷰의 최대 지적이었던 "퍼셉션이 폰 회전 모드·컨트롤러 포커스를 소유한다"는 `UWxBTService_LockOn` 이관(`f0aad4c3`)으로 해소됐고, 새 서비스는 해제 경로(틱·브랜치 이탈·`StopTree`·빙의 해제·컨트롤러 파괴)를 전부 `OnCeaseRelevant` 한 곳으로 모아 이전 설계의 비대칭을 없앴다. 이번 리뷰는 신규/변경 파일(`WxBTService_LockOn`, `WxAIPerceptionComponent`)을 전수 정독하고, 나머지 BT 노드 cpp 전부를 다시 읽었으며, 의심 지점(빙의 해제 순서, aux 노드 해제 통지, 추첨 메모리, `MoveTo` 우회 경로, 센스 중복 등록)은 UE 5.8 엔진 소스와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 Patrol·Wander 의 감속 GE 부여/제거가 통째로 중복된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:87`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:79`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:131`
- **범주**: 중복/복잡도
- **문제**: 두 태스크가 `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 쌍(`WxBTTask_Patrol.h:40`·`:49`, `WxBTTask_Wander.h:63`·`:72`), `MakeOutgoingSpec` → `SetSetByCallerMagnitude(SetByCaller_MoveSpeedScale)` → `ApplyGameplayEffectSpecToSelf` 부여 블록, `OnTaskFinished` 의 `GetOwningAbilitySystemComponent` → `RemoveActiveGameplayEffect` 제거 블록, 그리고 `GetStaticDescription` 의 "감속 GE 미지정" 분기(`WxBTTask_Patrol.cpp:74`, `WxBTTask_Wander.cpp:95`)까지 사실상 문자 단위로 같은 코드를 각자 들고 있다. 한쪽에만 적용한 수정(SetByCaller 태그 변경, 스택 처리, 핸들 유효성 강화)이 다른 쪽에 전파되지 않는다. 직전 리뷰에서도 지적됐고 그대로 남아 있다.
- **제안**: 베이스가 서로 달라(`UBTTask_MoveTo` vs `UBTTaskNode`) 중간 클래스는 부적절하다. 부여/제거 두 조각만 공유 헤더의 자유 함수로 빼는 정도가 침습이 적다. 반복이 2회에 그치는 동안은 현행 유지도 유효한 선택이다.
- **확신도**: 중간

### 2. 🟢 `bAvoidRepeat` 의 `LastChosenChild` 가 엔진의 탐색 재개 경로에서 갱신되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:125`
- **범주**: 버그/정확성
- **문제**: `LastChosenChild` 는 `GetNextChildHandler` 안에서만 기록된다. 그런데 UE 5.8 `UBTCompositeNode::GetNextChild` 는 `SearchData.SearchStart` 로 특정 노드에서 탐색을 재개하는 경우(`GetMatchingChildIndex`)와 `NodeMemory->OverrideChild` 가 걸린 경우 핸들러를 거치지 않고 자식을 직접 고른다. 그 경로로 실행된 자식은 기록에 남지 않아, 다음 추첨의 회피 기준이 실제 직전 자식과 어긋난다. 회피가 한 번 헛도는 정도라 영향은 작다.
- **제안**: 회피 정확도가 중요해지면 `NotifyChildExecution` 오버라이드에서 실제 실행된 자식 인덱스를 기록한다 — 세 경로가 모두 지나가는 지점이다. 그 전까지는 고칠 값어치가 크지 않다.
- **확신도**: 중간

### 3. 🟢 Wander: 이동 거리가 0이면 내비 검증이 통째로 무력해진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:40`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:49`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:61`
- **범주**: 버그/정확성
- **문제**: `:40` 의 주석이 "걸어갈 거리를 구할 수 없으면 길이 0 레이가 막힘으로 오지 않아 검증이 통째로 무력해진다" 고 경고하지만, 실제 가드는 `Movement` 널 여부만 본다(`:41~45`). `TravelDistance = GetMaxSpeed() * 배율 * Duration` 이 0이 되는 다른 경로 — `MovementMode == MOVE_None`(`UCharacterMovementComponent::GetMaxSpeed` 가 0을 돌려준다), SPD 를 0으로 만드는 속박 GE, 디자이너가 `Duration` 을 0으로 저작한 경우 — 에서는 `NavigationRaycast(Pawn, NavStart, NavStart)` 가 길이 0 레이라 막힘으로 판정되지 않아 8방향 전부가 무검증 통과한다. 대부분의 경우 그 상태에선 이동이 없어 무해하지만, 배회 도중(`Duration` 이 남은 사이) 이동 제약이 풀리면 검증되지 않은 방향으로 걸어가 내비메시를 벗어날 수 있다.
- **제안**: `TravelDistance` 가 유의미한 값인지(`> KINDA_SMALL_NUMBER`) 확인해 아니면 방향 탐색 전에 `Failed` 로 마감한다.
- **확신도**: 중간

### 4. 🟢 `SelfActor` 는 엔진이 이름·동기화를 이미 소유하는 예약 키다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:20`, `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h:34`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:38`
- **범주**: 중복/복잡도
- **문제**: UE 5.8 의 `FBlackboard::KeySelf` 가 정확히 `TEXT("SelfActor")`(`Engine/Source/Runtime/AIModule/Private/BehaviorTree/Blackboard/BlackboardKey.cpp:5`)이고, `AAIController::SetPawn` 이 빙의·해제마다 그 키를 현재 폰으로 덮어쓴다(`.../Private/AIController.cpp:583`). 즉 이 모듈은 엔진 예약 키 이름을 재선언하고 setter 까지 노출하고 있으며, 헤더 주석의 "SelfActor 의 SET/CLEAR 는 AIController 담당" 이라는 소유권 서술도 절반만 맞다. 실제로 유일하게 필요한 수동 개입은 "최초 빙의에서 Blackboard 가 `RunBehaviorTree` 이후에 생기는 바람에 엔진의 `SetPawn` 이 빈손으로 지나가는 순간" 뿐이고, `AWxAIController::OnUnPossess` 의 `SetSelfActor(BB, nullptr)`(`Source/WxGame/Controller/WxAIController.cpp:80`)는 곧이어 `Super::OnUnPossess` → `SetPawn(nullptr)` 이 같은 일을 하므로 순수 중복이다.
- **제안**: 상수는 `FBlackboard::KeySelf` 를 재사용하고, `SetSelfActor` 는 "BB 생성 직후 1회 보정" 이라는 유일한 용도를 헤더에 못박거나(그리고 unpossess 쪽 호출을 제거) 접근자 자체를 없앤다. `GetSelfActor` 는 타입 안전 조회로서 남길 값어치가 있으나, 유일한 소비자인 `WxBTService_TargetDistance.cpp:32` 는 BB 를 거치지 않고 `OwnerComp.GetAIOwner()->GetPawn()` 을 직접 써도 되는 자리다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 전체, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 소비자 쪽 `Source/WxGame/Controller/WxAIController.cpp`
- **교차 확인(엔진 소스 대조로 문제 없음을 확인한 항목)**:
  - 모듈 경계: Build.cs·uplugin 모두 `WxCore` 외 Wx 의존 없음. 코딩 규칙 6종(`Wx` prefix, 저작권 첫 줄, 람다, `Handle` prefix, `BlueprintCallable`, 인라인 정의) 전수 grep 결과 위반 0건 — `BlueprintCallable`·`FORCEINLINE`·람다는 모듈 전체에 단 한 건도 없고, 델리게이트 바인딩 6곳 모두 `Handle` prefix 함수다.
  - `UWxBTService_LockOn` 의 해제 대칭성: `AAIController::OnUnPossess` 가 `Super::OnUnPossess()`(→`SetPawn(nullptr)`)를 **먼저** 돌리고 그 뒤 `CleanupBrainComponent()` 를 부르므로, 해제 시점의 `GetPawn()` 은 이미 비어 있다 — 폰을 `FWxLockOnMemory::LockedOnPawn` 에 기록해 두는 설계가 이 순서 때문에 필요하며 정확하다. 그리고 `UBehaviorTreeComponent::StopTree` 가 활성 aux 노드 전부에 `WrappedOnCeaseRelevant` 를 돌리므로, 사망(`StopLogic`)·빙의 해제·컨트롤러 파괴가 모두 `OnCeaseRelevant` 한 곳으로 수렴한다. 직전 리뷰가 지적한 "폰만 살아남고 컨트롤러가 파괴되면 strafe 로 굳는다" 는 경로가 이 이관으로 닫혔다.
  - 노드 메모리 레이아웃: `UBTService` 는 `bTickIntervals` 때문에 `FBTAuxiliaryMemory` 특수 메모리를 갖지만 노드에 전달되는 `NodeMemory` 는 이미 그 뒤를 가리키므로, `GetInstanceMemorySize()==sizeof(FWxLockOnMemory)` + `CastInstanceNodeMemory` 조합이 올바르다. `UBTNode::InitializeMemory`/`CleanupMemory` 는 베이스가 비어 있어 `Super::` 대신 `InitializeNodeMemory<T>`/`CleanupNodeMemory<T>` 를 부르는 것이 엔진 관용이다.
  - `bCallTickOnSearchStart` 회피 판단: 폐기된 탐색은 aux 노드 업데이트를 적용하지 않으므로 그 틱에서 건 상태의 짝 통지가 오지 않는다는 코드 주석의 근거가 맞다.
  - `PostInitProperties` 의 `ConfigureSense` 3회 호출: `UAIPerceptionComponent::ConfigureSense` 가 센스 **클래스 단위**로 기존 엔트리를 교체하므로, 아키타입에서 복사된 `SensesConfig` 와 겹쳐도 중복 등록되지 않는다.
  - 생성자에서의 `OnTargetPerceptionUpdated.AddDynamic(this, ...)`: 네이티브 클래스 프로퍼티는 `CPF_Config` 가 아니면 `PostConstructLink` 에 오르지 않아 아키타입 복사에 덮이지 않는다.
  - `UWxBTComposite_RandomChoice` 의 베이스 선택: UE 5.8 `UBTComposite_Selector` 는 `GetNextChildHandler` 와 에디터 아이콘 외에 아무것도 오버라이드하지 않으므로, 상속해도 원치 않는 Selector 동작이 딸려오지 않는다.
  - `UWxBTTask_Patrol` 이 `bPatrolFinished` 에서 `Super::ExecuteTask` 없이 `InProgress` 를 반환하는 경로: `UBTTask_MoveTo` 는 `TickTask` 를 오버라이드하지 않아(`bNotifyTick` 미설정) 그 상태에서 엔진 틱이 스테일 메모리를 건드리지 않고, `AbortTask` 의 스테일 `MoveRequestID` 도 `UPathFollowingComponent::AbortMove` 의 현재 요청 ID 비교에서 걸러진다.
  - `UWxBTTask_ActivateAbility`: `FScopedAbilityListLock` 하에서 `GiveAbility` 가 지연되므로 순회 중 배열 재할당이 없고, 브로드캐스트 도중의 `OnAbilityEnded.Remove`/재바인드는 UE 멀티캐스트가 역순 순회로 공식 지원하는 사용이다(콜백이 추가한 인스턴스는 그 브로드캐스트에서 호출되지 않는다).
  - `UWxBTDecorator_BeyondLeash` 의 매 프레임 폴링 → `RequestExecution(this)`: `LowerPriority` 모드에서 하위 브랜치만 끊긴다. `GetLocationAtSplinePoint` 는 인덱스를 클램프하므로 `PatrolCursor` 가 앞서 나가도 널/원점 이동이 발생하지 않는다.
  - `EWxWanderDirection` 의 `Bitmask`/`BitmaskEnum` 메타: `UseEnumValuesAsMaskValuesInEditor` 기본값(false)에서 에디터가 `1 << EnumValue` 로 해석하므로 `UENUM` 에 `Bitflags` 를 달지 않아도 의도대로 동작한다.
  - 데드 코드 없음: `WxBlackboardKeys` 접근자·`OnTargetChanged`·`ForgetTargetActor`·`GetMoveMode` 모두 실제 호출부가 있다.
- **미검토 / 한계**: BT·Blackboard·스플라인 에셋의 실제 구성은 범위 밖이라, `UWxBTComposite_RandomChoice` 헤더가 스스로 경고한 "LowerPriority·Both 데코가 붙은 뒤쪽 자식이 추첨 결과를 끊는" 상황과 `UWxBTService_LockOn` 헤더가 전제한 "Gameplay 우선순위 포커스를 이 노드만 쓴다"(같은 브랜치에 `RotateToFaceBBEntry` 등이 없다)가 현재 트리에서 실제로 지켜지는지는 확인하지 않았다. `UWxPatrolComponent::ConfigureSpline` 이 `OnRegister` 에서 트랜잭션 없이 스플라인 포인트 타입을 덮어쓰는 것이 에디터 Undo 와 어떻게 상호작용하는지도 실행 검증하지 않았다. 멀티플레이는 BT·퍼셉션이 서버 전용이라는 전제만 확인했고 실제 복제 동작은 검증하지 않았다.

---
*문서 기준 커밋 `3d9e73c0` · 리뷰일 2026-09-04 · 소스 30파일 — `/module-review`로 갱신*
