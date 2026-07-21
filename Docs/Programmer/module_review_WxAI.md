# WxAI — 코드 리뷰

> 전반적으로 건강하다. 모듈 경계(WxCore 외 참조 없음)·명명·Copyright·Handle prefix·수명 관리(GAS 델리게이트·사망 태그 언바인드)가 규율 있게 지켜졌고 방어 코드와 주석이 충실하다. Perception 허브, BT Task/Decorator/Service/Composite, PatrolComponent, AnimNotify까지 29개 소스 전부를 통독했으며 GAS 발동·리시 복귀·가중 추첨 등 위험도 높은 경로를 깊게 봤다. 심각한 결함은 없고, 전투 분기의 논리 엣지 1건과 사소한 정리·설계 노트 3건만 남는다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 avoid-repeat가 "지금 유효한 유일한 자식"까지 제외해 컴포지트를 실패시킬 수 있음
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:61`, `:91`
- **범주**: 버그/정확성 (논리 엣지)
- **문제**: `GetNextChildHandler`는 `bAvoidRepeat`가 켜져 있으면 직전 선택 자식을 후보에서 무조건 `continue`로 제외한다(`:61`). 자식이 2개이고 직전에 child 0을 골랐는데 이번 진입에서 child 1의 조건 Decorator(예: `AttributeRatio` HP<0.5 발악기)가 false라면, child 1은 조건 필터로 빠지고 child 0은 avoid-repeat로 빠져 `Candidates.Num() == 0`이 되어 `ReturnToParent`(실패)를 반환한다(`:91`). child 0은 지금도 실행 가능한데 "직전에 썼다"는 이유만으로 컴포지트 전체가 실패하고 상위 Selector가 다른(하위) 브랜치로 넘어간다 — 반복이라도 하는 편이 나은 상황에서 공격 패턴을 아예 포기하는 셈이다.
- **제안**: 필터 결과가 avoid로 인해 비었을 때 avoid를 완화해 직전 자식을 다시 후보에 포함하는 폴백 한 단계를 두는 것을 검토. 강한 avoid가 의도라면 이 실패 거동을 헤더 주석의 시멘틱 설명에 명시하면 오해를 줄일 수 있다.
- **확신도**: 중간 (의도된 엄격 avoid이거나 상위 Selector 폴백에 의존하는 설계일 수 있음)

### 2. 🟢 WxBTTask_Wander.cpp의 미사용 include (`NavigationSystem.h`)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:5`
- **범주**: 중복/복잡도 (데드 코드)
- **문제**: `#include "NavigationSystem.h"`가 있으나 파일 어디에서도 내비게이션 시스템 API를 쓰지 않는다(이동은 `Pawn->AddMovementInput`로 처리). 컴파일 시간·의도 오해만 남긴다.
- **제안**: include 제거.
- **확신도**: 높음

### 3. 🟢 성공한 자극마다 무조건 TargetActor를 최신 감지 액터로 덮어씀 (다중 대상 시 타겟 진동)
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:82`
- **범주**: 설계/구조 (타겟 선정 정책)
- **문제**: `HandleTargetPerceptionUpdated`는 성공 자극이면 우선순위·거리 비교 없이 그 액터를 `SetTargetActor`로 확정한다. 감지 범위 안에 대상이 둘 이상이면 각 센스 갱신마다 타겟이 "가장 최근 감지된 액터"로 뒤바뀌어, 포커스·strafe 회전 모드가 대상 사이에서 진동할 수 있다. 단일 대상 위주 설계면 문제 없다.
- **제안**: 이미 유효 타겟이 있으면 유지(우선순위/거리/위협도 비교 후에만 스위치)하는 규칙을 두는 것을 검토.
- **확신도**: 낮음 (단일 타겟을 전제한 의도된 단순화일 수 있음)

### 4. 🟢 Wander가 내비메시 검증 없이 raw 이동 입력을 가함
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:95`
- **범주**: 성능/안전 (경계 검증)
- **문제**: 고른 월드 방향으로 `AddMovementInput`만 넣고 내비메시 투영·장애물 회피가 없다. 벽에 밀착하는 정도는 무해하나, 배치에 따라 절벽·내비메시 밖으로 걸어나갈 여지가 있다(오픈월드에서 QA 이슈 소지). 기본값이 짧은 시간(1s)·저속(0.3배)이라 노출은 제한적이다.
- **제안**: 목표 방향을 내비메시 위 지점으로 투영하거나(`ProjectPointToNavigation`) 이동 전 경계 체크를 두는 것을 검토.
- **확신도**: 낮음 (단순 idle 배회용으로 의도된 거동일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`(GAS 발동·동기 종료 방어·abort 시 델리게이트 해제 순서), `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`(가중 룰렛·조건 필터·avoid-repeat·노드 메모리 레이아웃), `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`(감지→타겟→인식→회전 모드, 사망 태그 바인드/언바인드, MinimalReplication 태그), `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`(매 틱 폴링·RequestExecution), `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`·`WxBTTask_ReturnHome.cpp`·`WxBTTask_Wander.cpp`(속도 캐시 복원·억제 연동·커서 진행)
- **훑은 파일**: `WxBlackboardKeys.h/.cpp`(타입드 accessor·Verify 진단), `WxPatrolComponent.h/.cpp`(스플라인 순회·컴포넌트 조회), `WxBTDecorator_AttributeRatio.cpp`, `WxBTService_TargetDistance.cpp`, `WxBTDecorator_RandomWeight.h/.cpp`, `WxAnimNotify_ReportNoise.cpp`, `WxTeamTypes.h`, `WxAIModule.cpp/.h`, `WxAI.Build.cs`, `README.md`
- **미검토 / 한계**: 런타임 동작(리시 왕복·타겟 진동)은 정적 판독만 했고 실제 플레이 검증은 하지 않음. `SelfActor`/`HomeLocation` 등 Blackboard 키 세팅과 팀(IGenericTeamAgentInterface) 배선, BT/BB 에셋의 FlowAbortMode 실제 설정값은 게임 콘텐츠(AIController)·에셋 측 책임이라 이 모듈 범위 밖이며, 그 전제가 지켜졌다고 보고 리뷰했다.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 29파일 — `/module-review`로 갱신*
