# WxAI — 코드 리뷰

> 상태 소유권과 수명주기가 전반적으로 잘 정리된 모듈이다. BT 노드마다 폰별 격리(`bCreateNodeInstance` / `GetInstanceMemorySize`)를 빠짐없이 챙겼고, 퍼셉션의 타겟 소실 구독·해제 경로와 어빌리티 발동/취소의 재진입 방어는 엔진 동작을 정확히 읽고 짠 흔적이 뚜렷하다. 커버리지: 소스 30파일 전부를 읽고, 위험도가 높은 `WxAIPerceptionComponent`·`WxBTTask_ActivateAbility`·`WxBTTask_MirrorAbility`·`WxBTComposite_RandomChoice`·`WxBTTask_Patrol` 은 UE 5.8 엔진 소스(`UAIPerceptionComponent::ConfigureSense`, `UBTNode::CastInstanceNodeMemory`, `UBehaviorTreeComponent::GetTaskStatus`/`StoreDebuggerRuntimeValues`, `UBTTask_MoveTo::AbortTask`, `UAbilitySystemComponent::NotifyAbilityEnded`)와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 미러링 대상이 어빌리티를 정상적으로 놓아도 태스크는 Failed 로 마감된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:190-194`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:230-232`
- **범주**: 버그/정확성
- **문제**: `TickTask` 는 대상이 식별 태그를 놓으면 `CancelAbilityHandle` 로 자기 어빌리티를 끊는다. 그 호출은 동기적으로 `OnAbilityEnded` 를 발행하고, 엔진은 그 통지에 `bWasCancelled = true` 를 실어 보낸다(`UAbilitySystemComponent::NotifyAbilityEnded`). `HandleAbilityEnded` 는 `bWasCancelled` 를 무조건 `EBTNodeResult::Failed` 로 매핑하므로, "대상을 끝까지 따라했다" 는 성공 경로가 BT 에는 실패로 전달된다. 190행 주석은 이 경로를 "abort 가 아니라 정상 마감" 이라고 적고 있어 의도와 실제 반환값이 어긋난다. 이 노드를 Sequence 에 두면 뒤따르는 형제가 매번 건너뛰어지고, Selector 에 두면 즉시 다음 폴백으로 떨어진다.
- **제안**: 이 경로에서만 서는 플래그(예: `bIsReleasingWithTarget`)를 세우고 `HandleAbilityEnded` 에서 `bWasCancelled` 대신 그 플래그를 보게 해 `Succeeded` 로 마감하거나, 반대로 Failed 가 의도라면 190행 주석과 헤더 클래스 주석에 "대상 추종 종료는 Failed 로 나간다" 를 명시해 저작자가 트리 구조를 그에 맞게 짜게 한다. 기존 `bIsRequestingAbort` 와 동일한 패턴이라 추가 비용은 거의 없다.
- **확신도**: 중간

### 2. 🟡 어빌리티 발동/중단/종료 프로토콜이 두 태스크에 통째로 복제돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:76-125`, `:141-178`, `:223-275` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:45-95`, `:102-139`, `:141-192` (헤더 상태 필드도 `WxBTTask_MirrorAbility.h:62-86` ↔ `WxBTTask_ActivateAbility.h:42-63` 로 동일)
- **범주**: 중복/복잡도
- **문제**: 태그를 어디서 얻느냐(저작값 `AbilityTag` vs 대상 ASC 폴링 `MirroredTag`)만 다르고, 그 뒤의 `FScopedAbilityListLock` 후보 순회·재발동 판별·`ActivationResult` 되감기·`CanBeCanceled` 즉시 마감·`GetTaskStatus` 로 abort/완료를 가르는 종료 처리까지 약 140줄이 주석 문구 차이를 빼면 문자 단위로 같다. 이 프로토콜은 이 모듈에서 가장 미묘한 코드(동기 종료·재발동·취소 거부의 3중 예외 처리)라, 한쪽에서 결함이 발견되면 반드시 양쪽을 같이 고쳐야 하는데 그 연결이 코드에 드러나 있지 않다. 실제로 발견 3처럼 한쪽에만 있는 가드가 이미 생겼다.
- **제안**: 두 노드 클래스는 그대로 두고(과거에 "기존 발동 태스크와 통합" 은 명시적으로 기각됐다), 발동 대상 태그를 순수 가상 함수로 뽑은 공통 베이스(`UWxBTTask_AbilityBase` 등)로 발동·중단·종료 구간만 끌어올리는 선을 검토한다. 통합이 아니라 프로토콜 한 벌만 공유하는 것이므로 기존 결정과 충돌하지 않는다. 이마저 원치 않으면 최소한 양쪽 헤더에 "이 프로토콜은 반대편 태스크와 쌍으로 유지한다" 는 주석을 남겨 다음 수정자가 한쪽만 고치지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 두 태스크를 별도 클래스로 유지하기로 한 선행 결정이 있다)

### 3. 🟢 MirrorAbility 의 TickTask 에는 AbortTask 가 가진 취소 거부 가드가 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:180-195` (없음) ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:163-175` (있음)
- **범주**: 버그/정확성
- **문제**: `AbortTask` 는 `CanBeCanceled()` 가 false 인 인스턴스를 만나면 경고를 남기고 즉시 마감하지만, `TickTask` 의 취소 경로에는 같은 검사가 없다. 취소를 거부하는 어빌리티를 따라하게 저작하면 대상이 태그를 놓은 뒤부터 그 어빌리티가 스스로 끝날 때까지 매 프레임 `CancelAbilityHandle` 이 아무 효과 없이 재호출되고, 로그도 남지 않아 저작 실수가 드러나지 않는다. (어빌리티는 결국 자기 조건으로 끝나므로 영구 정지는 아니다.)
- **제안**: `AbortTask` 의 `CanBeCanceled` 루프를 작은 헬퍼로 빼 `TickTask` 의 취소 직후에도 태우고, 거부가 확인되면 같은 경고를 한 번만 남기고 `FinishLatentTask` 로 마감한다.
- **확신도**: 중간

### 4. 🟢 BT 노드 3종에 NodeName 이 없어 그래프에 클래스명이 그대로 노출된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:11-14`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:13-19`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-16`
- **범주**: 설계/구조
- **문제**: 나머지 8개 노드는 생성자에서 `NodeName` 을 지정해 BT 에디터에 "Patrol", "Lock On", "Random Choice" 같은 이름으로 뜨는데, 이 셋만 비어 있어 엔진 폴백인 `WxBTTask_ActivateAbility` 형태의 타입명이 그대로 보인다. 이 모듈은 BT 저작 표면 그 자체이므로 일관성이 곧 사용성이다.
- **제안**: 각 생성자에 `NodeName = TEXT("Activate Ability")` / `TEXT("Wander")` / `TEXT("Attribute Ratio")` 한 줄씩 추가한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 Public 헤더 14종, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`
- **검증했으나 문제 없음(오탐 배제 기록)**:
  - 모듈 규칙 — `Build.cs`/`uplugin` 의존은 `WxCore` 하나뿐이고, `WxGameplayTags` 3개 태그 외 도메인 참조가 없다. `BlueprintCallable`·`FORCEINLINE`·인라인 정의·람다는 모듈 전체에 0건이고, 30파일 모두 첫 줄이 규정 저작권 문구다. `Handle` 접두사 규칙도 델리게이트 콜백 6종 전부 지켜졌다.
  - `UWxAIPerceptionComponent::PostInitProperties` 의 3회 `ConfigureSense` — 엔진 구현이 센스 클래스 단위로 기존 항목을 교체하므로 중복 등록이 생기지 않는다.
  - `UWxBTComposite_RandomChoice` 의 `FBTCompositeMemory` 확장 — `CastInstanceNodeMemory` 가 `sizeof(T) <= GetInstanceMemorySize()` 로 확장을 허용하며, `Super::InitializeMemory` 후에 `LastChosenChild` 를 쓰는 순서도 맞다.
  - `UWxBTTask_Patrol` 의 완주 시 `InProgress` 상주 — 5.8 의 `UBTTask_MoveTo` 는 `TickTask` 를 오버라이드하지 않아 `bNotifyTick` 이 꺼지고, `AbortTask` 도 무효 `MoveRequestID` 를 안전하게 통과한다. `GetLocationAtSplinePoint` 는 인덱스를 클램프하므로 커서가 경로 밖을 가리켜도 크래시하지 않는다.
  - 인스턴스 노드에서 `DescribeRuntimeValues`/`GetTaskStatus` 가 템플릿이 아니라 인스턴스를 대상으로 도는 점, BT 인스턴스 메모리가 `AddZeroed` 로 0 초기화되는 점(그래서 `FWxBeyondLeashMemory::bWasBeyond` 무초기화가 실제 위험이 아닌 점)을 모두 엔진 소스로 확인했다.
- **미검토 / 한계**: BT/Blackboard 애셋, `AWxAIController` 가 이 모듈의 노드를 어떤 트리 형태로 조립하는지는 리뷰 범위 밖이다. 발견 1의 실질 영향(Failed 가 어느 컴포짓 아래서 어떻게 소비되는지)과 `UWxBTComposite_RandomChoice` 가 조건 실패 자식 전부에 활성화 실패를 통지하는 설계의 실측 부작용은 애셋을 봐야 판정할 수 있어 코드 근거만으로 적었다. 리플리케이션·데디케이티드 서버 경로는 소음 보고(`UWxAnimNotify_ReportNoise`)의 `HasAuthority` 가드 외에는 검증하지 않았다.

---
*문서 기준 커밋 `a900118d` · 리뷰일 2026-09-05 · 소스 30파일 — `/module-review`로 갱신*
