# WxAI — AI 행동 시스템

> 적/NPC의 감지·행동을 담당한다. 엔진 Behavior Tree 위에 Wx 전용 태스크·데코레이터·서비스와 Perception 동기화, 정찰 경로 데이터를 얹는다.

## 책임
**담당**
- Perception(시각·청각·피격) 자극을 Blackboard `TargetActor`로 동기화하고 사망·소실 시 해제
- AI 락온(컨트롤러 포커스 + 폰 strafe 회전 모드)의 단독 소유(`UWxBTService_LockOn`) — 플레이어 락온은 [[WxCombat]] 소관으로 별개다
- BT 노드 라이브러리: 무작위 선택 Composite, 어빌리티 발동/정찰/배회/복귀 Task, 어트리뷰트·리시·가중치 Decorator, 타겟 거리·락온 Service
- 스플라인 기반 정찰 경로 데이터(`UWxPatrolComponent`)와 진행 규칙(PingPong/Loop/Once)
- Blackboard 키 이름·타입을 한 곳에 묶는 타입 안전 accessor(`WxBlackboardKeys`)
- 애님 노티파이로 청각 소음 이벤트 발행(`UWxAnimNotify_ReportNoise`)

**경계 (비담당)**
- 어빌리티·이펙트·어트리뷰트 정의는 [[WxCombat]] — WxAI는 의존하지 않고 `FGameplayTag`/`FGameplayAttribute`/`TSubclassOf<UGameplayEffect>`를 BT 에디터에서 디자이너가 주입
- AIController·Blackboard 에셋·BT 에셋 저작 및 폰 아키타입은 이 모듈 밖(콘텐츠/게임 모듈)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 자극 → `TargetActor` 동기화의 시작점. 타겟 획득/해제 | `Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxBTService_LockOn` | 부착한 브랜치 동안 `TargetActor`를 컨트롤러 포커스·폰 회전(strafe) 모드에 반영 | `Source/WxAI/Public/WxBTService_LockOn.h` |
| `WxBlackboardKeys` | BT 노드·Perception이 공유하는 키 이름/accessor 허브 | `Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTComposite_RandomChoice` | 조건·가중치로 자식을 추첨하는 Composite. `RandomWeight`·`bAvoidRepeat`와 함께 동작 | `Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | GAS 어빌리티를 태그로 발동하고 종료 결과를 노드 결과로 반환 | `Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_Patrol` | `UBTTask_MoveTo` 확장. `UWxPatrolComponent` 경로를 따라 정찰 커서 진행 | `Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커(HomeLocation)에서 리시 반경 이탈 판정, 복귀 브랜치 게이팅 | `Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터(무상태). 부착 부모에 붙여 폰이 참조 | `Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 AI 행동은 엔진 베이스(`UBTTaskNode`/`UBTDecorator`/`UBTService`/`UBTCompositeNode`, 이동류는 `UBTTask_MoveTo`)를 상속하고 `Wx` 접두사로 만든다. 노드별 상태는 `bCreateNodeInstance`(폰 인스턴스) 또는 노드 메모리 struct(예: `FWxBTRandomChoiceMemory`, `FWxBeyondLeashMemory`)로 보관한다.
- Blackboard는 직접 `GetValueAs`/`SetValueAs`를 쓰지 말고 `WxBlackboardKeys` accessor를 경유한다. 새 키는 이 네임스페이스에 이름·타입·accessor를 함께 추가한다.
- WxCombat 의존 없이 전투 자산을 쓰는 노드(`ActivateAbility`·`Patrol`·`Wander`·`AttributeRatio`)는 `FGameplayTag`/`FGameplayAttribute`/`GameplayEffect`를 `UPROPERTY`로 노출해 BT 에디터에서 디자이너가 지정하게 한다.
- 정찰 경로는 데이터 주도: 적이 부착된 액터(스포너 등)에 `UWxPatrolComponent`를 달고 `MoveMode`로 순회 규칙을 정한다. 진행 상태는 경로가 아니라 BT 태스크가 폰별로 소유한다.

## 여기서부터 읽어라
1. `Source/WxAI/Public/WxBlackboardKeys.h` — 모듈 전체가 공유하는 Blackboard 계약. 키 소유권 분담이 헤더에 정리돼 있다.
2. `Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟이 어떻게 잡히고 풀리는지(감지 경로·사망/소실 해제)의 진입점. 그 타겟을 바라보는 상태는 `WxBTService_LockOn.h`가 이어받는다.
3. `Source/WxAI/Public/WxBTComposite_RandomChoice.h` — 무작위 선택 시멘틱과 Decorator 연동(조건/가중치)이 Selector와 어떻게 다른지.

## 관련
- 상위: BT/Blackboard/AIController 에셋을 저작하는 콘텐츠·게임 모듈이 이 노드 라이브러리를 소비한다. 전투 자산 주입 대상은 [[WxCombat]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 28파일 — `/readme-writer`로 갱신*
