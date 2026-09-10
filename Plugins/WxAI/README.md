# WxAI — AI 시스템

> 적 폰의 감지·타겟 선정·행동(Behavior Tree)을 담당한다. 무엇을 감지하고 누구를 노릴지, 정찰·리시 복귀·어빌리티 발동을 어떤 트리로 엮을지를 데이터 주도로 제공한다.

## 책임
**담당**
- 퍼셉션(시각·청각·피격)으로 적대 대상을 감지하고, 감지 결과 중 하나를 TargetActor로 승격
- Behavior Tree용 커스텀 Task/Service/Decorator/Composite 노드 묶음 (정찰·배회·리시 복귀·어빌리티 발동/모방·락온·거리 갱신 등)
- Blackboard 키 이름·값 타입을 한곳에 묶는 타입 안전 accessor (`WxBlackboardKeys`)
- 폰별 감각 수치·행동 자산을 캐릭터 상속과 분리해 제공하는 컴포넌트
- 스플라인 기반 정찰 경로 데이터, 애님 노티파이 기반 소음 방출

**경계 (비담당)**
- AIController 자체와 락온 대상 보관 — Source/WxGame `AWxAIController`, [[WxCombat]] `UWxLockOnComponent`에 위임. 이 모듈은 그 대상을 "어떻게 바라볼지"만 정한다
- 어트리뷰트(HP 등) 정의 — [[WxCombat]] 소유. `UWxBTDecorator_AttributeRatio`는 어떤 Attribute를 비교할지 BT 에디터에서 디자이너가 직접 지정한다(WxCombat 미의존)
- 어빌리티 실행 로직 — GAS/[[WxCombat]]. BT 노드는 태그로 발동을 요청·관찰만 한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 노드·컨트롤러가 공유하는 Blackboard 키·accessor의 단일 정의(데이터 버스) | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIPerceptionComponent` | 시각·청각·피격 감지. 컨트롤러에 붙어 폰 종류를 가리지 않음 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxBTService_UpdateTargetActor` | 감지된 후보 중 하나를 TargetActor로 기록(타겟 선정의 유일한 주체) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxAIBehaviorComponent` | 폰별 행동 자산·감각 수치를 상속과 분리해 제공 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터(무상태) + 순회 규칙 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTService_LockOn` | 컨트롤러 포커스와 폰 strafe 회전 모드를 한 쌍으로 소유해 TargetActor 반영 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | 태그로 GAS 어빌리티 발동을 요청하고 종료까지 추적 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |

## 확장 포인트 / 규약
- **새 BT 노드**: 엔진 베이스(`UBTService`/`UBTTaskNode`/`UBTDecorator`/`UBTCompositeNode`)를 상속하고 `WxBT<종류>_<이름>` 규칙을 따른다. 인스턴스별 상태는 노드 메모리 구조체(`FWx...Memory`)로 두며, Composite 파생은 반드시 베이스 메모리(`FBTCompositeMemory`) 뒤에 자체 필드를 배치한다(`UWxBTComposite_RandomChoice` 참고).
- **Blackboard 접근**: `GetValueAs`/`SetValueAs` 직접 호출 대신 `WxBlackboardKeys`의 accessor를 쓴다. Blackboard 에셋에 같은 이름·타입의 키가 등록돼 있어야 하며, Float 부재값은 `NoTargetDistance`로 기록해 "무한히 멀다"로 읽히게 한다.
- **데이터 주도 설정**: 정찰 경로는 폰이 부착된 액터(스포너 등)의 `UWxPatrolComponent`에서, 감각 수치·행동 트리는 폰의 `UWxAIBehaviorComponent`에서 읽는다. 어빌리티/어트리뷰트는 BT 에디터에서 태그·핸들로 지정한다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 노드가 공유하는 데이터 버스. 키의 SET/CLEAR 소유권 분담(컨트롤러 vs BT 노드)이 여기 정리돼 있어 제어 흐름의 지도 역할을 한다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` → `WxBTService_UpdateTargetActor.h` — "감지 → 타겟 선정"의 데이터 흐름과 두 컴포넌트의 책임 경계.
3. `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` — 폰별 설정이 어떻게 컨트롤러/퍼셉션으로 흘러 들어가는지(빙의 시점 읽기).

## 관련
- 상위: `Source/WxGame/Controller/WxAIController.h` (`AWxAIController`) — 이 모듈의 컴포넌트·행동 트리를 실제로 구동하는 소비자
- 협력: [[WxCombat]] — 어빌리티/어트리뷰트/락온 대상 보관

---
*문서 기준 커밋 `ffc6360` · 생성일 2026-09-10 · 소스 38파일 — `/readme-writer`로 갱신*
