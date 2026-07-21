# WxAI — 코드 리뷰

> 방어적으로 잘 짜인 건강한 모듈이다. GAS 발동 시 배열 재할당·동기 종료, 룰렛 부동소수 경계, 노드 인스턴싱 등 알려진 함정을 코드·주석이 이미 대부분 다룬다. 이번 리뷰는 15개 cpp 전부와 주요 헤더를 읽었고, Perception 허브·리시 메커니즘·GAS 발동 Task를 깊게 봤다. BT 에셋 배선(FlowAbortMode 실제 설정값 등)은 C++ 밖이라 확인 못 함.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

- `WxBTDecorator_AttributeRatio`는 헤더가 "FlowAbortMode로 실시간 재평가"를 약속하지만 이를 촉발할 폴링·관찰자가 없어, HP 등 어트리뷰트 게이트로 진행 중 행동을 중단시키려는 의도가 실제로는 동작하지 않는다.
- `UWxAIPerceptionComponent`의 Sight/Hearing 센스는 컨트롤러가 `ApplySenseSettings`를 호출해야만 등록되어, 누락 시 시각·청각이 조용히 죽는다.

## 발견

### 🟡 AttributeRatio: FlowAbortMode로 실시간 재평가가 안 됨(촉발 메커니즘 부재)
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:16`, 구현 전체 `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`
- **범주**: 버그/정확성
- **문제**: 헤더는 "실시간 재평가가 필요하면 FlowAbortMode를 LowerPriority/Self/Both로 설정하라"고 안내한다. 그러나 이 데코는 ASC 어트리뷰트를 직접 읽을 뿐 `TickNode`도, Blackboard 키 관찰자도, `RequestExecution` 호출도 없다(`CalculateRawConditionValue`만 오버라이드). 엔진에서 일반 `UBTDecorator`는 FlowAbortMode만으로 스스로 재평가하지 않는다 — 관찰 대상 키 변경이나 틱 폴링이 있어야 abort가 촉발된다. 같은 상황(값 관찰용 BB 키 없음)을 `WxBTDecorator_BeyondLeash`는 `TickNode` 폴링 + `RequestExecution`(`WxBTDecorator_BeyondLeash.cpp:51-64`)으로 명시 해결했는데, AttributeRatio에는 그 장치가 빠져 있다. 결과적으로 "HP 30% 이하로 떨어지면 진행 중인 공격을 끊고 도주" 같은 브랜치는 현재 노드가 자연 종료되기 전엔 발동하지 않는다. 진입 시점 정적 게이트(예: RandomChoice 후보 필터)로는 정상 동작한다.
- **제안**: BeyondLeash처럼 `TickNode`에서 조건 변화 감지 시 `RequestExecution`을 호출하거나, 실시간 재평가를 지원하지 않는다면 헤더의 FlowAbortMode 관련 문장을 정정한다.
- **확신도**: 중간

### 🟡 Perception: Sight/Hearing 센스가 외부 ApplySenseSettings 호출에 의존(누락 시 무감지)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:34-42`(Damage만 `PostInitProperties`에서 `ConfigureSense`), `52-70`(Sight/Hearing은 `ApplySenseSettings`에서만 `ConfigureSense`)
- **범주**: 설계/구조
- **문제**: 생성자는 세 센스의 Config 서브오브젝트만 만들고, `ConfigureSense`(퍼셉션 시스템 등록)는 Damage만 `PostInitProperties`에서 수행한다. Sight/Hearing은 컨트롤러가 `ApplySenseSettings`를 호출할 때 비로소 등록된다. 즉 컨트롤러가 이 계약을 지키지 않으면 AI는 데미지만 감지하고 시각·청각이 조용히 죽으며, 컴파일·로그 어디에도 티가 안 난다. 파라미터 주입이 필요한 Sight/Hearing과 무파라미터 Damage의 비대칭은 이해되나, 실패가 완전 무음이라 위험하다.
- **제안**: `ApplySenseSettings` 미호출을 감지할 수 있게 등록 여부를 `ensure`/경고 로그로 드러내거나, 합리적 기본값으로 Sight/Hearing도 최소 1회 `ConfigureSense`해 두고 이후 주입으로 갱신한다.
- **확신도**: 낮음(문서화된 의도적 계약)

### 🟡 Perception: 새로 감지된 적이 무조건 현재 TargetActor를 덮어씀(멀티 타겟 시 타겟 튐)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:82-85`
- **범주**: 설계/구조
- **문제**: `HandleTargetPerceptionUpdated`는 감지 성공이면 억제 중이 아닌 한 항상 `SetTargetActor(Actor)`로 타겟을 그 액터로 교체한다. 적을 여럿(co-op의 여러 플레이어, 다수 적) 감지하는 상황에서 가장 최근 감지된 대상으로 타겟이 계속 바뀌어, 보스가 대상 사이를 왕복하며 포커스/strafe 회전이 튈 수 있다. `State.InCombat`을 `MinimalReplication`으로 여러 클라에 복제하는 구조상 멀티플레이 시나리오가 전제되어 있어 실제로 드러날 여지가 있다.
- **제안**: 현재 유효 타겟이 있으면 유지하고(신규 감지는 타겟 없을 때만 채택), 또는 거리/위협도 기준의 선택 규칙을 둔다. 최신 감지 우선이 의도라면 그대로 두되 문서에 명시.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 🟢 SetTargetActor: non-Character 폰은 포커스가 발행되지 않음
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:225-230`
- **범주**: 버그/정확성
- **문제**: 회전 모드 발행 전 `Cast<ACharacter>` 실패 시 early return 한다. 바로 위 주석은 "포커스는 MovementComponent가 없어도 발행"을 강조하지만(그건 실제로 지켜짐 — SetFocus가 Movement 가드 밖), `ACharacter`가 아닌 폰은 그 이전에 return 되어 `SetFocus`/`ClearFocus`조차 호출되지 않는다. 회전 모드는 Character 전용이라 스킵이 타당하나, 포커스(AIController 기능)는 Character가 아니어도 유효하다. 현재 모든 적이 Character라 실질 영향은 없다.
- **제안**: 포커스 발행을 `!Character` 가드 밖으로 빼거나(회전 모드만 Character 가드), 현 동작이 의도면 주석에 non-Character 스킵을 한 줄 명시.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `WxBTTask_ReturnHome.cpp`, `WxBTTask_Wander.cpp`, `WxBTService_TargetDistance.cpp`, `WxBTDecorator_RandomWeight.cpp`, `WxBlackboardKeys.cpp`, `WxAnimNotify_ReportNoise.cpp`, `WxAIModule.cpp`, 및 동명 헤더 전부
- **미검토 / 한계**: BT/Blackboard 에셋 배선(FlowAbortMode 실제 설정, SelfActor 키 세팅 주체, 브랜치 우선순위)은 C++ 범위 밖이라 검증하지 못했다. `INIT_DECORATOR_NODE_NOTIFY_FLAGS`/`DoDecoratorsAllowExecution(GetActiveInstanceIdx())` 등 엔진 내부 계약은 빌드·관용 근거로 정상 가정했다. MaxWalkSpeed 캐시/복원의 Wander↔Patrol 중복은 프로젝트 방침(인플레이스 선호)에 따라 발견에서 제외했다. 규칙 위반(Prefix·Copyright·Handle 접두사·BlueprintCallable·람다·플러그인 의존)은 스캔했으며 위반 없음.

---
*문서 기준 커밋 `702fc70f` · 리뷰일 2026-07-22 · 소스 29파일 — `/module-review`로 갱신*
