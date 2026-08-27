# WxAI — AI 행동/인지 시스템

> 적 AI의 Behavior Tree 노드(Task·Decorator·Service·Composite), 지각(Perception), Blackboard 키 규약, 정찰 경로를 제공하는 도메인 플러그인이다. AIController·EnemyController·Spawner 등 폰/컨트롤러 실체는 이 플러그인 밖(게임 모듈)에 있고, 여기서는 그것들이 조립해 쓰는 부품을 담는다.

## 책임
**담당**
- 커스텀 BT 노드: 어빌리티 발동, 정찰, 리시(leash) 복귀, 배회, 어트리뷰트 비율/리시/가중치 데코레이터, 가중 무작위 선택 Composite, 타겟 거리 서비스
- Perception: Sight/Hearing/Damage 감지를 Blackboard `TargetActor`로 동기화하고 인식(전투 태그)·회전 모드를 발행
- Blackboard 키 규약(`WxBlackboardKeys`): 키 이름·타입을 accessor로 묶어 타입 오용 방지
- 정찰 경로 데이터(`UWxPatrolComponent`, 스플라인), 소음 발생 AnimNotify, 팀 열거형(`EWxTeam`)

**경계 (비담당)**
- 전투 어빌리티·이펙트·어트리뷰트의 정의와 실행 — GAS로 위임. WxAI는 [[WxCombat]]에 의존하지 않으며, `FGameplayAttribute`/`UGameplayEffect`는 디자이너가 BT 에디터에서 직접 지정한다
- AIController·EnemyController·Spawner·폰 클래스 — 게임 모듈(WxGame) 소유. 이 플러그인의 노드/컴포넌트를 조립해 사용

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 지각→`TargetActor` 동기화, 인식/회전 모드 발행, 억제(disengage) 제어 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | BB 키 이름·타입 accessor 규약(SelfActor/TargetActor/HomeLocation/PatrolTargetLocation/TargetDistance) | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTComposite_RandomChoice` | 조건·가중치로 후보를 걸러 무작위 1개 실행(Selector 폴백 없음) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커에서 `LeashRadius` 이탈 판정으로 복귀 브랜치 게이팅(폴링+RequestExecution) | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 리시 복귀 이동 + 복귀 중 재감지 억제(`SetTargetingSuppressed`) | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h` |
| `UWxBTTask_ActivateAbility` | `AbilityTag`로 어빌리티 발동, 종료 결과를 노드 결과로 변환 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_Patrol` | `MoveTo` 상속, 도착 시 `UWxPatrolComponent` 커서 진행 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로(상태 없음), `FindPatrolComponent`로 조회 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`/`UBTDecorator`/`UBTService`/`UBTCompositeNode`)를 상속하고 `Wx` 접두사로 추가. BB 접근은 `GetValueAs`/`SetValueAs` 직접 호출 대신 `WxBlackboardKeys` accessor를 통한다 — 키가 에셋에 없거나 타입이 어긋나면 accessor가 경고 로그로 드러낸다(Shipping 제외)
- 리시 복귀는 데코(`BeyondLeash`, 게이팅)와 Task(`ReturnHome`, 완료 판정 단독 소유)의 분업으로 구성 — 복귀 완료는 항상 Task가 정한다
- 감속·어트리뷰트 등 GAS 자원은 코드에 하드코딩하지 않고 BT 에디터 노출 프로퍼티(예: `WxEffect_MoveSpeedScale`, `WxCombatAttributeSet::HP`)로 디자이너가 지정 — WxAI↔WxCombat 무의존을 유지하는 규약
- 정찰 경로는 `AWxSpawner`에 `UWxPatrolComponent`를 추가하고, 스폰된 적이 `FindPatrolComponent`로 조회. 진행 커서는 경로가 아니라 폰별 Task 메모리가 소유

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 획득/유지/소실·인식·억제의 전체 수명이 클래스 doc에 정리돼 있어 이 모듈의 상태 기계를 먼저 잡기 좋다
2. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 노드/컴포넌트가 공유하는 BB 계약. 어느 주체가 어느 키를 쓰는지 여기서 파악
3. `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` + `WxBTTask_ReturnHome.h` — 리시 복귀 분업 구조

## 관련
- 상위: [[WxCore]](공용 정의 의존) · 전투 위임 [[WxCombat]]

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 29파일 — `/readme-writer`로 갱신*
