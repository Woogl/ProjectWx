# WxAI — 코드 리뷰

> 엔진 BT/Perception 위에 얇게 얹힌 노드 묶음으로, 노드 메모리 레이아웃·델리게이트 재진입·엔진 계약(StopTree 의 aux 통지, ConfigureSense 중복 방지, MoveTo 의 알림 플래그, 디버거의 노드 인스턴스 해석)을 엔진 소스와 대조해도 어긋나는 곳이 없다. 직전 리뷰의 🟡 2건(취소 거부 어빌리티에서의 abort 무한 대기, 완주한 Patrol 이 디버거에서 구분되지 않던 문제)은 모두 처리됐고, 지금 남은 것은 전부 "알고 둔 것" 급이다. 이번 리뷰는 15개 헤더와 15개 cpp 전부 · `WxAI.Build.cs` · `uplugin` 을 읽고, 판단이 갈리는 지점(어빌리티 취소 가능 여부, BT 노드 메모리·알림 플래그, 블랙보드 관찰자 재진입, `ForgetActor` 의 빌드별 가용성)은 UE 5.8 엔진 소스와 `Plugins/WxCombat` 실제 구현으로 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟢 취소 거부 가드가 NonInstanced 어빌리티는 걸러내지 못한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:126`
- **범주**: 버그/정확성
- **문제**: `AbortTask` 는 `ActiveSpec->GetAbilityInstances()` 를 훑어 `CanBeCanceled()` 가 false 인 인스턴스를 찾으면 즉시 `Aborted` 로 마감한다. 그런데 이 배열은 `EGameplayAbilityInstancingPolicy::NonInstanced` 어빌리티에서는 항상 비어 있다 — 엔진의 `UAbilitySystemComponent::CancelAbilitySpec` 도 그 경우 CDO 의 `CancelAbility` 를 직접 부르고, 그 안에서 `CanBeCanceled()` 가 false 면 조용히 아무 일도 하지 않는다. 즉 NonInstanced + 취소 거부 조합이면 가드가 그냥 지나가고 `InProgress` 를 반환해, 이 가드가 막으려던 "트리가 Aborting 에 갇힘" 이 그대로 재현된다. 다만 이 Task 는 `AbilityTag` 로 임의의 `UGameplayAbility` 를 물 수 있는 반면, 현재 프로젝트의 어빌리티는 전부 `UWxAbilityBase`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:21`, `InstancedPerActor`) 파생이라 지금은 발현하지 않는다.
- **제안**: 인스턴스 순회 앞에 `ActiveSpec->Ability` 의 `GetInstancingPolicy() == NonInstanced && !ActiveSpec->Ability->CanBeCanceled()` 한 줄을 같은 마감 경로로 붙이면 구멍이 닫힌다. 또는 헤더 계약(`Public/WxBTTask_ActivateAbility.h:19`)에 "이 Task 는 인스턴스형 어빌리티를 전제한다"를 덧붙인다.
- **확신도**: 중간

### 2. 🟢 AttributeRatio 는 실행 중 재평가 경로가 아예 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:11`
- **범주**: 설계/구조
- **문제**: 생성자가 `bAllowAbortLowerPri`·`bAllowAbortChildNodes` 를 모두 false 로 잠가 `FlowAbortMode` 에 `None` 외의 값을 저작할 수 없다. 어트리뷰트 변경 구독도, `BeyondLeash` 같은 틱 폴링도 없으므로 이 데코레이터는 트리 재탐색 시점(=현재 Task 가 끝날 때)에만 판정된다. 짝이 되는 `BeyondLeash` 가 폴링+`RequestExecution` 으로 즉시성을 확보한 것과 대비되며, "HP 50% 아래로 떨어지면 즉시 페이즈 전환" 류 요구가 생기면 이 노드로는 불가능하다. 직전 리뷰에서도 같은 지적이 있었고 헤더(`Public/WxBTDecorator_AttributeRatio.h:11`)에는 아직 그 제약이 적혀 있지 않다.
- **제안**: 지금 필요 없다면 그대로 두되, 어빌리티 실행 중에는 뒤집히지 않는다는 점을 헤더 주석에 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 BeyondLeash 가 틱 간격 없이 매 프레임 폴링한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:86`
- **범주**: 성능/안전
- **문제**: 관찰자로 등록된 동안 `TickNode` 가 매 프레임 `CalculateRawConditionValue` 를 돌리고, 그 안에서 `IsExecutingBranch`(인스턴스 스택 탐색) → `GetBlackboardComponent` → `GetLocationFromEntry` 를 매번 수행한다. 개체당 비용은 작지만 오픈월드에서 동시 활성 적 수에 그대로 비례한다. 리시 이탈은 0.1s 지연이 문제 되지 않는 판정이다.
- **제안**: 생성자에서 `bTickIntervals = true` 로 두고 `TickNode` 끝에서 `SetNextTickTime(NodeMemory, 0.1f)` 를 호출하면 된다(`UBTAuxiliaryNode` 가 특수 메모리를 알아서 잡아 준다). 같은 모듈의 `UWxBTService_LockOn`·`UWxBTService_TargetDistance` 가 이미 0.1s 주기를 쓰고 있어 결과 지연 폭도 일관된다.
- **확신도**: 중간

### 4. 🟢 Patrol·Wander 의 감속 GE 부여·제거 로직이 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:56`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:84`
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`·`MoveSpeedEffect` UPROPERTY 쌍, `MakeOutgoingSpec` → `SetSetByCallerMagnitude(SetByCaller_MoveSpeedScale)` → `ApplyGameplayEffectSpecToSelf` 부여 블록, `OnTaskFinished` 의 제거 블록(`WxBTTask_Patrol.cpp:97`, `WxBTTask_Wander.cpp:136`), `GetStaticDescription` 의 "감속 GE 미지정" 분기까지 네 덩어리가 두 파일에 동일하게 존재한다. 상속 계보가 갈려(`UBTTask_MoveTo` vs `UBTTaskNode`) 자연스러운 공통 베이스가 없다는 점이 원인이다.
- **제안**: 프로젝트 방침상 소규모 중복은 용인하는 쪽이므로 그대로 둬도 무방하다. 다만 SetByCaller 태그나 제거 시점 규칙이 바뀌면 두 곳을 함께 고쳐야 한다는 사실만 한쪽 주석에 남겨 두면 좋다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 나머지 `Public/*.h` 전부, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 경계 확인용 `Source/WxGame/Controller/WxAIController.cpp`
- **직전 리뷰 이후 해소된 항목**: (1) `AbortTask` 무한 대기 — `WxBTTask_ActivateAbility.cpp:124` 의 취소 거부 검사와 경고 로그, 헤더 계약 명시로 데드락 경로가 닫혔다(위 발견 1의 NonInstanced 구멍만 남음). (2) 완주한 Patrol 이 디버거에서 이동 중과 구분되지 않던 문제 — `GetStaticDescription`(`WxBTTask_Patrol.cpp:75`)의 "브랜치 점유" 문구와 `DescribeRuntimeValues`(같은 파일 78)로 그래프·디버거 양쪽에 노출됐다. 엔진의 `StoreDebuggerRuntimeValues` 가 `HasInstance()` 를 보고 노드 인스턴스에 위임하므로, `bCreateNodeInstance` 노드인데도 폰별 커서가 정확히 표시된다.
- **검증한 규칙**: 모듈 의존성은 `WxCore` + 엔진 모듈뿐이라 플러그인 DAG 위반 없음. `BlueprintCallable`·`FORCEINLINE`·인라인 정의·람다는 모듈 전체에서 0건. 델리게이트 콜백은 전부 `Handle` prefix(`HandleTargetPerceptionUpdated`, `HandleAbilityEnded`, `HandlePawnHit`, `HandlePossessedPawnChanged` 등). 30개 소스 전부 첫 줄이 저작권 표기이며 개행은 CRLF 로 통일됐다(6개 파일에 UTF-8 BOM 이 남아 있으나 규칙 위반은 아니다).
- **미검토 / 한계**: BT/Blackboard/AIController 에셋 저작 내용(어느 트리가 어떤 노드를 어떤 `FlowAbortMode`·`Interval` 로 배치했는지)은 C++ 밖이라 보지 않았다. `UWxBTTask_Patrol` 의 "완주 후 브랜치 점유" 는 의도된 설계로 확인해 발견에서 뺐지만, 그 결과가 실제 트리에서 어떤 폴백을 막는지는 에셋에 달려 있다. 멀티플레이는 서버 전용 실행 전제만 코드로 확인했고 실측하지 않았다.

---
*문서 기준 커밋 `303d8d7f` · 리뷰일 2026-09-05 · 소스 30파일 — `/module-review`로 갱신*
