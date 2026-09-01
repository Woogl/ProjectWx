# WxAI — AI 행동 시스템

> 적 AI의 Behavior Tree 노드(Task·Decorator·Service·Composite), 인지(Perception), Blackboard 키 계약, 정찰 경로를 제공한다. AIController·캐릭터·전투 규칙은 담지 않고, BT 에디터에서 조립할 수 있는 재사용 부품과 인지→Blackboard 동기화만 책임진다.

## 책임
**담당**
- Behavior Tree 커스텀 노드: 어빌리티 발동·정찰·배회·리시 복귀 Task, 리시 이탈·어트리뷰트 비율 Decorator, 타겟 거리 Service, 가중 무작위 선택 Composite
- 인지 동기화: 시각·청각·피격 자극을 Blackboard `TargetActor`로 합성하고 타겟 소실을 판정 (`UWxAIPerceptionComponent`)
- Blackboard 키 이름·타입 계약과 타입 안전 accessor (`WxBlackboardKeys`)
- 정찰 경로 데이터(스플라인)와 순회 규칙 (`UWxPatrolComponent`)
- 팀 구분 enum (`EWxTeam`)과 애님 노티파이 기반 소음 발생

**경계 (비담당)**
- AIController·AI 캐릭터 클래스, `SelfActor`/`HomeLocation` 키 세팅 → `WxGame`(WxAIController 등)
- 어빌리티·이펙트·어트리뷰트의 정의와 실제 전투 규칙 → [[WxCombat]] (BT 노드는 GAS 태그/이펙트 클래스를 데이터로만 참조하며 WxCombat에 링크하지 않음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 키 이름·타입 계약 허브. 어떤 주체가 어떤 키를 SET/CLEAR하는지 헤더 주석에 정리됨 | `Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIPerceptionComponent` | 세 감각을 `TargetActor` 하나로 합성, 타겟 소실 판정·회전 모드 발행 | `Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxBTComposite_RandomChoice` | 조건 통과 자식만 후보로 가중 무작위 선택 (Selector와 다른 시멘틱) | `Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTDecorator_RandomWeight` | 위 Composite의 자식 추첨 가중치 운반(조건 평가 아님) | `Source/WxAI/Public/WxBTDecorator_RandomWeight.h` |
| `UWxBTTask_ActivateAbility` | GAS 어빌리티를 BT에서 발동하고 종료 결과를 노드 결과로 매핑 | `Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커에서 리시 이탈 판정·폴링 재평가. `UWxBTTask_ReturnHome`과 복귀 브랜치를 이룸 | `Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로(상태 없는 순수 데이터). 커서는 `UWxBTTask_Patrol`이 폰별로 소유 | `Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`/`UBTDecorator`/`UBTService`/`UBTCompositeNode`)를 상속해 추가한다. Blackboard 키에 접근할 때는 `GetValueAs`를 직접 쓰지 말고 `WxBlackboardKeys` accessor를 거쳐 타입 오용을 막는다.
- WxAI는 `WxCombat`에 의존하지 않으므로, 감속 이펙트(`MoveSpeedEffect`)·어트리뷰트(`Attribute`/`MaxAttribute`)·어빌리티 태그(`AbilityTag`)는 코드가 아니라 디자이너가 BT 에디터에서 `TSubclassOf`·`FGameplayAttribute`·`FGameplayTag`로 직접 지정한다.
- 정찰: 적이 부착된 액터(스포너 또는 레벨 배치 액터)에 `UWxPatrolComponent`를 붙이면 `UWxBTTask_Patrol`이 `FindPatrolComponent`로 찾아 따른다. 없으면 그 적은 정찰하지 않는다.
- 인지→Blackboard는 서버 권한 흐름이다. 소음 발생(`UWxAnimNotify_ReportNoise`)과 피격 보고(Damage 센스)는 서버 전용이며, 피아 판정은 `FGenericTeamId::GetAttitude`로 한다(팀은 `EWxTeam`).

## 여기서부터 읽어라
1. `Source/WxAI/Public/WxBlackboardKeys.h` — 키 계약과 SET/CLEAR 소유 주체가 정리돼 있어, 노드·인지가 Blackboard로 어떻게 통신하는지의 지도가 된다.
2. `Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 획득/소실이 전체 AI 흐름의 트리거이므로 인지 경로부터 잡는다.
3. `Source/WxAI/Private/WxBTComposite_RandomChoice.cpp` — 후보 필터링과 가중 추첨 로직(RandomWeight Decorator와의 협업)이 여기서 결정된다.

## 관련
- 상위: `WxGame`의 AIController·캐릭터(WxAIController 등)가 이 모듈의 노드와 컴포넌트를 조립해 사용한다. 어빌리티/어트리뷰트/이펙트는 [[WxCombat]] 자산을 BT 에디터에서 데이터로 물린다.

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 28파일 — `/readme-writer`로 갱신*
