# WxAI — 코드 리뷰

> 상태 소유권·수명주기 처리가 이 저장소에서 가장 잘 정리된 모듈 중 하나다. BT 노드마다 폰별 격리(`bCreateNodeInstance` / `GetInstanceMemorySize`)를 빠짐없이 챙겼고, 퍼셉션의 구독 해제 경로와 어빌리티 발동/취소의 3중 예외(동기 종료·재발동·취소 거부)는 엔진 동작을 정확히 읽고 짠 흔적이 뚜렷하다. 커버리지: 소스 32파일을 모두 읽었고, `WxAIPerceptionComponent`·`WxBTTask_ActivateAbility`·`WxBTTask_MirrorAbility`·`WxBTComposite_RandomChoice`·`WxBTTask_Patrol`·`WxBTService_LockOn`·`WxBTDecorator_BeyondLeash` 는 호출 경로와 엔진 계약(`OnTargetPerceptionUpdated` 발화 조건, `UBTTask_MoveTo::AbortTask`, `UBTCompositeNode` 메모리 레이아웃, `FScopedAbilityListLock`)을 따라가며 깊게 봤다. 지난 리뷰의 미러링 마감 결함과 `TickTask` 취소 거부 가드 누락은 두 건 모두 수정된 것을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 타겟을 해제해도, 이미 계속 보고 있는 다른 적대 액터로는 다시 붙지 못한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:93-100` (획득 게이트), `:111-120` (사망 해제), `:122-126` (파괴 해제), `:167-173` (빙의 변경 해제)
- **범주**: 버그/정확성
- **문제**: 타겟 획득은 `HandleTargetPerceptionUpdated` 하나뿐이고, 그 함수는 `OnTargetPerceptionUpdated` 가 올 때만 돈다. 엔진은 이 델리게이트를 **감지 상태가 바뀔 때만** 방송한다(매 갱신마다 오는 것은 별개의 `OnTargetPerceptionInfoUpdated` 다). 그래서 계속 시야에 들어와 있는 액터는 이미 "감지 중" 상태를 유지하는 동안 통지를 한 번도 더 만들지 않는다.
  결과적으로 타겟을 해제하는 세 경로(사망 태그·`EndPlay`·빙의 변경)에서 폰 앞에 **이미 보이고 있던 다른 적대 액터**가 있어도 타겟이 빈 채로 남는다. 구체적 시나리오: 두 명이 한 적을 상대하다 한 명이 죽으면, 바로 옆에 서 있는 생존자가 시야 안에 있는데도 적은 타겟이 없어 전투 브랜치에서 빠진다. 생존자가 공격(Damage 센스)하거나 소리(Hearing)를 내거나 시야를 끊었다 다시 들어와야 재획득된다.
  `UWxBTTask_ReturnHome` 경로만 이 함정을 피한다 — `ForgetTargetActor` 가 `ForgetActor` 로 퍼셉션 기록을 지워 다음 시야 질의가 새 상태 변화로 잡히기 때문이다(`WxAIPerceptionComponent.cpp:102-110`). 나머지 해제 경로에는 그 정리가 없다. 헤더 주석(`WxAIPerceptionComponent.h:54`)은 시체가 계속 감지 상태로 남는다는 점을 이미 알고 있는데, cpp 주석(`WxAIPerceptionComponent.cpp:113`)의 "다음 감지 자극에서 정상적으로 재획득한다" 는 그 전제와 어긋난다.
- **제안**: `SetTargetActor(nullptr)` 로 타겟을 비운 직후(또는 세 Handle 함수의 공통 지점에서) `GetCurrentlyPerceivedActors` 로 현재 감지 중인 목록을 한 번 훑어, 살아 있고 적대적인 첫 액터를 곧바로 다음 타겟으로 승격시킨다. 자극을 기다리지 않고 이미 가진 정보만 다시 읽는 것이라 추가 감지 비용이 없다.
- **확신도**: 중간

### 2. 🟡 어빌리티 발동/중단/종료 프로토콜이 두 태스크에 통째로 복제돼 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:46-125`, `:141-177`, `:249-299` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:16-95`, `:102-139`, `:141-192` (헤더 상태 필드도 `WxBTTask_MirrorAbility.h:65-89` ↔ `WxBTTask_ActivateAbility.h:42-63` 로 동일)
- **범주**: 중복/복잡도
- **문제**: 태그를 어디서 얻느냐(저작값 `AbilityTag` vs 대상 ASC 폴링 `MirroredTag`)만 다르고, 그 뒤의 `FScopedAbilityListLock` 후보 순회·재발동 판별·`ActivationResult` 되감기·`CanBeCanceled` 즉시 마감·`GetTaskStatus` 로 abort/완료를 가르는 종료 처리까지 약 150줄이 주석 문구 차이를 빼면 문자 단위로 같다. 이 프로토콜은 이 모듈에서 가장 미묘한 코드라, 한쪽에서 결함이 나오면 반드시 양쪽을 같이 고쳐야 하는데 그 연결이 코드에 드러나 있지 않다. 실제로 지난 리뷰가 지적한 "MirrorAbility 쪽에만 `CanBeCanceled` 가드가 없다" 는 결함이 정확히 이 방식으로 생겼다(지금은 수정됨).
- **제안**: 두 노드 클래스는 그대로 두고(과거에 "기존 발동 태스크와 통합" 은 명시적으로 기각됐다), 발동 대상 태그만 순수 가상 함수로 뽑은 공통 베이스(`UWxBTTask_AbilityBase` 등)로 발동·중단·종료 구간을 끌어올린다. 통합이 아니라 프로토콜 한 벌만 공유하는 것이므로 기존 결정과 충돌하지 않는다. 이마저 원치 않으면 최소한 양쪽 헤더에 "이 프로토콜은 반대편 태스크와 쌍으로 유지한다" 는 주석을 남겨 다음 수정자가 한쪽만 고치지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 두 태스크를 별도 클래스로 유지하기로 한 선행 결정이 있다)

### 3. 🟢 BT 노드 3종에 NodeName 이 없어 그래프에 클래스명이 그대로 노출된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:11-14`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:13-19`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-16`
- **범주**: 설계/구조
- **문제**: 나머지 노드는 생성자에서 `NodeName` 을 지정해 BT 에디터에 "Patrol", "Lock On", "Random Choice", "Mirror Ability" 로 뜨는데, 이 셋만 비어 있어 엔진 폴백인 `WxBTTask_ActivateAbility` 형태의 타입명이 그대로 보인다. 이 모듈은 BT 저작 표면 그 자체이므로 일관성이 곧 사용성이다. (지난 리뷰에서 지적됐으나 그대로 남아 있다.)
- **제안**: 각 생성자에 `NodeName = TEXT("Activate Ability")` / `TEXT("Wander")` / `TEXT("Attribute Ratio")` 한 줄씩 추가한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 Public 헤더 16종, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`
- **검증했으나 문제 없음(오탐 배제 기록)**:
  - 모듈 규칙 — `Build.cs`/`uplugin` 의존은 `WxCore` 하나뿐이고, 실제 include 도 `WxGameplayTags.h` 외 도메인 참조가 없다. `BlueprintCallable`/`BlueprintPure`·`FORCEINLINE`·헤더 인라인 정의·람다는 모듈 전체에 0건, 32파일 모두 첫 줄이 규정 저작권 문구다. 델리게이트 콜백 6종 모두 `Handle` 접두사를 지켰고, 타입 접두사도 전부 `Wx` 다.
  - 지난 리뷰 지적 2건이 수정된 것을 확인 — `UWxBTTask_MirrorAbility::TickTask` 는 이제 `Succeeded` 로 마감하고(`:220`), 같은 자리에 `CanBeCanceled` 거부 경고도 들어갔다(`:203-217`).
  - `UWxBTDecorator_AttributeRatio`·`UWxBTDecorator_RandomWeight` 가 `bAllowAbortLowerPri`/`bAllowAbortChildNodes` 를 모두 끈 것은 실수가 아니라 정합적이다 — 어트리뷰트·가중치 변화는 관찰자로 잡을 수단이 없으므로 엔진이 `FlowAbortMode` 를 `None` 으로만 저작하게 강제한다.
  - `UWxBTTask_Patrol` 의 완주 시 `InProgress` 상주 — `UBTTask_MoveTo` 의 태스크 메모리는 `InitializeNodeMemory` 로 0 초기화돼 `bWaitingForPath` 가 꺼져 있고, `AbortTask` 도 무효 `MoveRequestID` 를 안전하게 통과한다. 감속 GE 도 이 분기 앞에서 되돌아가 붙지 않는다.
  - `UWxBTService_LockOn` 의 포커스/회전 모드 왕복 — 최초 진입·재타겟·폰 교체·타겟 소실 네 경로를 모두 따라가 `ReleaseLockOn` 의 멱등성과 아키타입 복원이 성립함을 확인했다. `bCallTickOnSearchStart` 를 쓰지 않는 판단도 근거가 맞다.
  - `UWxBTComposite_RandomChoice` 의 `FBTCompositeMemory` 확장, 가중치 0 후보 제외와 회피 완화 순서, `TotalWeight > 0` 보장, 부동소수 폴백(`:114`) 모두 정합적이다.
  - `UWxBTTask_ActivateAbility`/`MirrorAbility` 의 `AddUObject` 구독은 약참조라 노드 인스턴스나 ASC 가 먼저 사라져도 댕글링이 되지 않는다. `CleanUp` 이 ASC 소멸 후 해제를 건너뛰는 것도 안전하다.
- **미검토 / 한계**: BT/Blackboard 애셋과 `AWxAIController` 가 이 노드들을 어떤 트리 형태로 조립하는지는 리뷰 범위 밖이라, 발견 1의 실제 체감(전투 브랜치 게이트가 `TargetActor` 를 어떻게 읽는지)과 `UWxBTComposite_RandomChoice` 가 조건 실패 자식 전부에 활성화 실패를 통지하는 설계의 부작용은 코드 근거로만 적었다. `UWxAIPerceptionComponent::PostInitProperties` 의 `ConfigureSense` 3회 호출은 지난 리뷰가 엔진 소스로 검증한 결과를 그대로 받아들였고 이번에 재검증하지 않았다. 리플리케이션·데디케이티드 서버 경로는 `UWxAnimNotify_ReportNoise` 의 `HasAuthority` 가드 외에는 보지 않았다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 32파일 — `/module-review`로 갱신*
