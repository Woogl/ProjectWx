# WxAI — AI 시스템

> 적 폰의 감지·추적·정찰·리시 복귀를 Behavior Tree 노드와 Perception/Patrol 컴포넌트로 구현한다. 트리를 짜기 위한 커스텀 BT 노드 팔레트와, 그 노드들이 공유하는 Blackboard 규약이 이 모듈의 핵심이다.

## 책임
**담당**
- 감지 → 타겟팅: 시각·청각·피격 자극을 `UWxAIPerceptionComponent`가 Blackboard `TargetActor`로 동기화. 사망·파괴·리시 복귀에서만 타겟 해제.
- 커스텀 BT 노드: 정찰/배회/복귀/어빌리티 발동 Task, 거리·리시·어트리뷰트비율 판정, 가중치 무작위 선택 Composite.
- 정찰 경로 데이터: `UWxPatrolComponent`(스플라인)가 순회 규칙만 제공하고 진행 커서는 BT Task가 폰별로 소유.
- Blackboard 키 이름·타입을 한 곳에 묶은 타입 안전 accessor(`WxBlackboardKeys`).
- 애님 노티파이 소음 발생(`UWxAnimNotify_ReportNoise`).

**경계 (비담당)**
- 어빌리티 발동·어트리뷰트 정의: GAS(GameplayAbilities)에 위임. `UWxBTTask_ActivateAbility`는 태그로 발동만 걸고, 어트리뷰트는 디자이너가 BT에서 지정(WxAI는 [[WxCombat]]에 의존하지 않음).
- BT/Blackboard 에셋 저작, 실제 트리 구성: 콘텐츠(BP/에셋) 측.
- 이동/경로탐색/도착 판정: 엔진 `UBTTask_MoveTo`·NavMesh에 위임.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→`TargetActor` 동기화의 허브. 회전 모드·타겟 소실 감시 발행 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | Perception·BT 노드가 공유하는 Blackboard 키/accessor 규약 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxPatrolComponent` | 정찰 경로(스플라인) + MoveMode 순회 규칙, 무상태 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTTask_Patrol` | `MoveTo` 상속, 도착 시 정찰 커서를 폰별로 진행 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxBTDecorator_BeyondLeash` | 리시 이탈 폴링→`RequestExecution`으로 복귀 브랜치 게이팅 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTComposite_RandomChoice` | 조건·가중치를 반영한 자식 무작위 선택(Selector 폴백 없음) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | 태그로 GAS 어빌리티 발동, 종료 결과를 노드 결과로 변환 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |

## 확장 포인트 / 규약
- Blackboard 규약: 새 키는 `WxBlackboardKeys`에 이름+accessor로 추가하고 GetValueAs/SetValueAs 직접 호출을 피한다. Object 키는 nullptr setter가 Clear와 동치, Float `TargetDistance`는 타겟 부재 시 `NoTargetDistance`를 써 근거리 비교가 통과하지 않게 한다.
- 새 BT 노드: `UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`(또는 엔진 파생) 상속. Composite에서 자체 노드 메모리를 쓸 땐 베이스 메모리 뒤에 배치(`FWxBTRandomChoiceMemory` 패턴).
- 리시(leashing): `BeyondLeash` 데코가 브랜치를 열고 `UWxBTTask_ReturnHome` Task가 복귀 완료를 단독 판정 — 복귀 중엔 데코가 참을 유지해 경계 왕복을 막는다.
- 무작위 선택 가중치: 자식에 `UWxBTDecorator_RandomWeight`를 붙여 조율(조건 데코 아님, 없으면 1.0).
- GAS 연동: 어빌리티/어트리뷰트는 태그·`FGameplayAttribute`로 데이터 주도. 코드 의존은 GameplayAbilities까지만.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 노드가 공유하는 데이터 계약. 먼저 읽어야 나머지 흐름이 보인다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 감지→타겟팅 제어 흐름의 시작점.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` + `WxBTTask_ReturnHome.h` — 데코/Task 협업(리시)의 대표 예.

## 관련
- 상위: 적 캐릭터·Experience를 조립하는 [[WxGame]] 및 GameFeature 콘텐츠 플러그인. 어빌리티 태그·어트리뷰트는 [[WxCombat]]과 데이터로만 맞물린다(코드 의존 없음).

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 29파일 — `/readme-writer`로 갱신*
