# WxAI — 적 AI 행동/인지

> 적 폰의 지각(Perception)과 Behavior Tree 실행 재료를 제공한다. 무엇을 감지하고(TargetActor) 어디까지 쫓다 어디로 돌아오는지(leash/return), 그리고 BT가 쓰는 커스텀 노드·블랙보드 규약을 담당한다.

## 책임
**담당**
- Perception 셋업(Sight/Hearing/Damage)과 감지 결과를 Blackboard `TargetActor`로 동기화, 전투 인식(`State.InCombat`) 발행
- 리시(leash) 이탈 판정과 홈 복귀 브랜치, 복귀 중 재-어그로 억제
- BT 커스텀 노드(Task/Service/Decorator/Composite)와 정찰 경로(Spline) 데이터
- Blackboard 키 이름·타입 규약(`WxBlackboardKeys`)

**경계 (비담당)**
- 어빌리티 발동·어트리뷰트 정의는 [[WxCombat]] 소관 — 여기선 태그/어트리뷰트 핸들을 BT 에디터에서 받아 GAS API로 호출만 한다(모듈 의존 없음)
- AIController·폰·스폰(AWxSpawner)·Blackboard/BT 에셋은 이 모듈이 아닌 소비 측([[WxGame]] 등)에 있다
- 네이티브 Gameplay Tag 선언은 [[WxCore]](`WxGameplayTags`)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→`TargetActor`→인식/회전모드 발행의 중심. 타겟 수명·억제 로직 전부 | `Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | BB 키 이름/타입 accessor 규약. 모든 노드가 여기로 BB를 읽고 쓴다 | `Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커에서 LeashRadius 이탈 판정, 복귀 브랜치 게이팅 | `Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 리시 복귀 실행. 이동 시작 시 퍼셉션에 억제 지시, 종료 시 해제 | `Source/WxAI/Public/WxBTTask_ReturnHome.h` |
| `UWxBTTask_ActivateAbility` | `AbilityTag`로 GAS 어빌리티 발동, 종료 결과를 노드 결과로 매핑 | `Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 조건 통과 자식 중 가중치 추첨 1개 실행(`RandomWeight`와 짝) | `Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxPatrolComponent` | 정찰 경로(Spline). 순수 경로 데이터, 진행 커서는 `WxBTTask_Patrol`이 폰별 소유 | `Source/WxAI/Public/WxPatrolComponent.h` |
| `EWxTeam` | 피아 구분 팀 enum(Player/Enemy/Neutral) | `Source/WxAI/Public/WxTeamTypes.h` |

## 확장 포인트 / 규약
- BB 접근은 반드시 `WxBlackboardKeys` accessor 경유(키 이름·타입 오용 방지). Blackboard 에셋에 동일 이름 키가 등록돼 있어야 한다.
- WxCombat에 의존하지 않으므로, 어빌리티 태그·어트리뷰트(HP/MaxHP 등)·감속 GE(`WxEffect_MoveSpeedScale`)는 디자이너가 BT 에디터에서 직접 지정한다(`WxBTTask_ActivateAbility`, `WxBTDecorator_AttributeRatio`, `WxBTTask_Wander`/`_Patrol`).
- 새 커스텀 노드는 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`)를 상속해 `Wx` 접두사로 추가.
- 타겟/인식/억제는 `UWxAIPerceptionComponent` 단일 지점에서만 바뀐다 — 리시 이탈 판정만 BT로 이관됨. 인식 발행은 서버 전용(MinimalReplication 태그).

## 여기서부터 읽어라
1. `Source/WxAI/Public/WxAIPerceptionComponent.h` — 감지→타겟→인식→복귀 억제가 엮인 흐름의 허브. 헤더 주석이 수명 규칙 전체를 설명한다
2. `Source/WxAI/Public/WxBlackboardKeys.h` — 노드들이 공유하는 데이터 계약. 누가 어느 키를 쓰는지 여기 정리됨
3. `Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` + `WxBTTask_ReturnHome.h` — 리시/복귀 협업(데코가 게이팅, Task가 완료·억제 소유)

## 관련
- 상위: [[WxGame]](AIController·폰·스폰·BT/BB 에셋이 이 모듈의 노드·컴포넌트를 소비), [[WxCombat]](어빌리티·어트리뷰트·GE), [[WxCore]](`WxGameplayTags`)

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 29파일 — `/readme-writer`로 갱신*
