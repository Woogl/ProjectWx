# WxAI — AI 시스템

> 적 폰의 지능을 이루는 런타임 부품 모음이다. 감지(Perception)로 타겟을 잡고, Behavior Tree 노드·정찰 경로·블랙보드 규약으로 그 타겟을 상대하는 행동을 조립한다.

## 책임
**담당**
- 시각·청각·피격 감지를 Blackboard `TargetActor` 로 동기화 (`UWxAIPerceptionComponent`)
- Behavior Tree 커스텀 노드: Task(어빌리티 발동·정찰·복귀·배회), Service(락온·타겟 거리), Decorator(리시 이탈·속성 비율·랜덤 가중), Composite(랜덤 선택)
- 스플라인 기반 정찰 경로 데이터와 순회 규칙 (`UWxPatrolComponent`)
- Blackboard 키 이름·타입 accessor 규약 (`WxBlackboardKeys`)
- 애님 프레임에서의 소음 발생 (`UWxAnimNotify_ReportNoise`)

**경계 (비담당)**
- AIController 본체와 락온 대상 결정 — 외부 `AWxAIController`/`UWxLockOnComponent` 소유 (이 모듈은 그 대상을 "어떻게 바라볼지"만 정함)
- 어빌리티/이펙트 정의·전투 규칙 — [[WxCombat]] (BT 노드는 `FGameplayTag`·`TSubclassOf<UGameplayEffect>` 를 저작값으로 받아 ASC에 위임)
- 무엇을 켤지(Experience)·Blackboard/BehaviorTree 애셋 저작

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→`TargetActor` 발행의 유일 지점, 타겟 소실 감시 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | 키 이름·타입 accessor를 한 곳에 묶은 네임스페이스 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxPatrolComponent` | 정찰 경로 데이터·순회 규칙(상태 없음), 부착 부모에서 조회 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTService_LockOn` | 컨트롤러 포커스+폰 회전모드를 한 쌍으로 소유 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | `AbilityTag`로 ASC 어빌리티 발동, 종료를 결과로 매핑 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_Patrol` | `UBTTask_MoveTo` 상속, 도착 시 정찰 커서 진행(폰별) | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커에서 리시 반경 이탈 폴링 판정 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxAnimNotify_ReportNoise` | 애님 프레임에서 청각 자극 방출(서버 전용) | `Plugins/WxAI/Source/WxAI/Public/WxAnimNotify_ReportNoise.h` |

## 확장 포인트 / 규약
- **BT 노드 추가**: 엔진 베이스(`UBTTaskNode`·`UBTService`·`UBTDecorator`·`UBTCompositeNode`)를 상속해 `Wx` 접두사로 추가. 노드별 상태는 `bCreateNodeInstance` 또는 `GetInstanceMemorySize`/`InitializeMemory` 로 폰별 격리 — 같은 트리를 여러 폰이 공유·리스폰해도 안전해야 한다.
- **Blackboard 접근**: `GetValueAs`/`SetValueAs` 직접 호출 대신 `WxBlackboardKeys` accessor를 쓴다. 키 이름은 Blackboard 애셋에 같은 이름으로 등록돼 있어야 하며, accessor는 Blackboard non-null을 전제(호출부 가드)한다.
- **데이터 주도 저작**: 정찰은 적이 부착된 액터에 `UWxPatrolComponent`를 붙여 스플라인 포인트로 지정. 어빌리티/속도 이펙트는 BT 에디터에서 `FGameplayTag`/`GameplayEffect` 클래스로 지정(WxAI는 WxCombat에 의존하지 않으므로 코드가 아닌 저작으로 연결).
- **권한**: 소음 방출(`ReportNoiseEvent`)은 서버 전용.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 획득/소실의 단일 진입점. AI 흐름의 시작.
2. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 BT 노드가 공유하는 데이터 계약. 키 소유권 분담이 헤더 주석에 정리돼 있다.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` — 포커스/회전이라는 분산된 상태를 한 노드가 소유하는 경계 설계의 대표 예.

## 관련
- 상위: 적 폰의 AIController·Experience에서 이 모듈의 컴포넌트와 BT 애셋을 조립해 사용. 저작값을 통해 [[WxCombat]](어빌리티·이펙트)과 느슨하게 연결된다.

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 30파일 — `/readme-writer`로 갱신*
