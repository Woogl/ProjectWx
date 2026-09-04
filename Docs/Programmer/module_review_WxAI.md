# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹힌 노드 묶음으로, 노드 메모리 레이아웃·델리게이트 수명·엔진 계약(FindChildToExecute, 특수 노드 메모리, Damage 센스 리스너 역추적)을 엔진 소스와 대조해도 어긋나는 곳이 거의 없다. 주석이 "왜 이렇게 했는지"를 남겨 둬 재검토 비용이 낮다. 이번 리뷰는 `*.Build.cs`·`uplugin`·15개 헤더 전부와 15개 cpp 전부를 읽고, 판단이 갈리는 지점(BT 노드 메모리·서비스 특수 메모리·Damage/Hearing 센스 전달 경로·Ability 취소 정책)은 UE 5.8 엔진 소스와 `Plugins/WxCombat` 실제 구현으로 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 취소를 거부하는 어빌리티에서 AbortTask 가 무기한 대기한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:100`
- **범주**: 버그/정확성
- **문제**: `AbortTask` 는 `CancelAbilityHandle` 로 취소를 요청한 뒤 스펙이 여전히 Active 면 `InProgress` 를 반환하고 종료 통지를 기다린다. 그런데 `UAbilitySystemComponent::CancelAbilitySpec` 은 `CanBeCanceled()` 가 false 인 인스턴스를 로그만 남기고 건너뛴다. 이 프로젝트의 `UWxAbilityBase::CanBeCanceled()`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:173`)는 `Override` 그룹에서 false 를 돌려주므로, `WxAbility_Death`·`WxAbility_Groggy`·`WxAbility_Finisher`·`WxAbility_HitReact`·`WxAbility_PlayMontageOnce` 계열을 이 Task 로 물리면 취소 요청이 조용히 무시되고 BT 브랜치는 그 어빌리티가 스스로 끝날 때까지 Aborting 에 갇힌다. 스스로 끝나지 않는 어빌리티(사망 등)면 그 트리는 영구 정지한다.
- **제안**: `AbilityTag` 가 지목한 어빌리티가 취소 불가일 때 최소한 `LogWxAI` 경고를 남기거나, `AbortTask` 진입 시 취소가 실제로 먹혔는지(스펙 Active 여부)를 근거로 즉시 `Aborted` 로 마감하는 폴백을 둔다. 저작 계약("Override 그룹 어빌리티는 이 Task 로 발동하지 않는다")을 헤더 주석에 못 박는 것만으로도 대부분 예방된다.
- **확신도**: 중간

### 2. 🟡 정찰이 끝난 Patrol Task 가 브랜치를 영구 점유한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:46`
- **범주**: 설계/구조
- **문제**: `bPatrolFinished` 면 `Super::ExecuteTask` 도 부르지 않고 `InProgress` 를 반환한다. 이 Task 는 `bNotifyTick` 도 없고 latent 완료 경로도 없으므로 스스로는 절대 끝나지 않는다 — 상위 우선순위 데코레이터가 abort 해 줄 때만 브랜치를 놓는다. `Once` 모드에서는 `OnTaskFinished` 의 `GetMoveMode() != Once` 가드 때문에 abort 후에도 `bPatrolFinished` 가 유지되어, 재진입할 때마다 다시 눌러앉는다. 결과적으로 같은 Selector 아래에 둔 배회·대기 폴백은 Once 완주 이후 영영 실행되지 않고, 같은 Sequence 의 뒤따르는 형제도 마찬가지다. 헤더(`Public/WxBTTask_Patrol.h:16`)에 의도로 명시돼 있고 Failed/Succeeded 대안을 왜 버렸는지도 적혀 있으나, "정찰이 끝나면 AI 가 통째로 멈춘다" 는 결과는 BT 저작자가 디버거에서 원인을 찾기 어려운 형태다.
- **제안**: 유지한다면 `GetStaticDescription` 에 "완주 시 브랜치 점유" 를 노출해 BT 그래프에서 보이게 하거나, 대기 상태를 별도의 명시적 Wait/Idle Task 로 분리해 Patrol 은 Failed 로 브랜치를 넘기게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 AttributeRatio 는 실행 중 재평가 경로가 아예 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:11`
- **범주**: 설계/구조
- **문제**: 생성자가 `bAllowAbortLowerPri`·`bAllowAbortChildNodes` 를 모두 false 로 잠가 `FlowAbortMode` 에 `None` 외의 값을 저작할 수 없다. 어트리뷰트 변경 구독도, `BeyondLeash` 같은 틱 폴링도 없으므로 이 데코레이터는 트리 재탐색 시점(=현재 Task 가 끝날 때)에만 판정된다. 짝이 되는 `BeyondLeash` 가 폴링+`RequestExecution` 으로 즉시성을 확보한 것과 대비되며, "HP 50% 아래로 떨어지면 즉시 페이즈 전환" 류 요구가 생기면 이 노드로는 불가능하다.
- **제안**: 지금 필요 없다면 그대로 두되, 어빌리티 실행 중에는 뒤집히지 않는다는 점을 헤더 주석에 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 BeyondLeash 가 틱 간격 없이 매 프레임 폴링한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:86`
- **범주**: 성능/안전
- **문제**: 관찰자로 등록된 동안 `TickNode` 가 매 프레임 `CalculateRawConditionValue` 를 돌리고, 그 안에서 `IsExecutingBranch`(인스턴스 스택 탐색) → `GetBlackboardComponent` → `GetLocationFromEntry` 를 매번 수행한다. 개체당 비용은 작지만 오픈월드에서 동시 활성 적 수에 그대로 비례한다. 리시 이탈은 0.1s 지연이 문제 되지 않는 판정이다.
- **제안**: 생성자에서 `bTickIntervals = true` 로 두고 `TickNode` 끝에서 `SetNextTickTime(NodeMemory, 0.1f)` 를 호출하면 된다(`UBTAuxiliaryNode::GetSpecialMemorySize` 가 특수 메모리를 알아서 잡아 준다).
- **확신도**: 중간

### 5. 🟢 Patrol·Wander 의 감속 GE 부여·제거 로직이 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:56`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:84`
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 쌍, `MakeOutgoingSpec` → `SetSetByCallerMagnitude(SetByCaller_MoveSpeedScale)` → `ApplyGameplayEffectSpecToSelf` 부여 블록, `OnTaskFinished` 의 제거 블록, `GetStaticDescription` 의 "감속 GE 미지정" 분기까지 네 덩어리가 두 파일에 동일하게 존재한다. 상속 계보가 갈려(`UBTTask_MoveTo` vs `UBTTaskNode`) 자연스러운 공통 베이스가 없다는 점이 원인이다.
- **제안**: 프로젝트 방침상 소규모 중복은 용인하는 쪽이므로 그대로 둬도 무방하다. 다만 SetByCaller 태그나 제거 시점 규칙이 바뀌면 두 곳을 함께 고쳐야 한다는 사실만 한쪽 주석에 남겨 두면 좋다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 나머지 `Public/*.h` 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 경계 확인용 `Source/WxGame/Controller/WxAIController.cpp`
- **검증한 규칙**: 모듈 의존성은 `WxCore` + 엔진 모듈뿐이라 플러그인 DAG 위반 없음. `BlueprintCallable`·`FORCEINLINE`·인라인 정의·불필요한 람다는 모듈 전체에서 0건. 델리게이트 콜백은 전부 `Handle` prefix(`HandleTargetPerceptionUpdated`, `HandleAbilityEnded`, `HandlePawnHit` 등). 30개 소스 전부 첫 줄이 저작권 표기다(단, 6개 파일은 UTF-8 BOM + LF, 나머지 24개는 BOM 없음 + CRLF 로 인코딩·개행이 섞여 있다 — 내용상 규칙 위반은 아니나 diff 노이즈 요인).
- **미검토 / 한계**: BT/Blackboard/AIController 에셋 저작 내용(어느 트리가 어떤 노드를 어떤 `FlowAbortMode` 로 배치했는지)은 C++ 밖이라 보지 않았다. 따라서 발견 1·2의 실제 발현 여부는 에셋에 달려 있다. 멀티플레이 시나리오(서버 전용 실행 전제)는 코드상 전제만 확인했고 실측하지 않았다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 30파일 — `/module-review`로 갱신*
