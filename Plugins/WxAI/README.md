# WxAI — AI 시스템

> 적 폰의 행동을 굴리는 재료를 모은 런타임 플러그인. Behavior Tree 노드(Task·Service·Decorator·Composite), 퍼셉션·피격 자극 배선, Blackboard 키 규약, 정찰 경로 데이터를 제공한다.

## 책임
**담당**
- BT 노드 저작 재료: 어빌리티 발동/미러, 정찰·복귀·배회 이동, 타겟 갱신·거리 갱신·락온 서비스, 리시·속성비·가중치 데코레이터, 무작위 선택 컴포짓
- 캐릭터가 AI로 구동될 때 필요한 것(BT 애셋·감각 수치·피격 자극 보고)을 폰에 얹는 `UWxAIBehaviorComponent`
- Blackboard 키 이름·값 타입을 한곳에 묶은 accessor 규약(`WxBlackboardKeys`)과 키 소유 분담
- 정찰 경로 데이터(`UWxPatrolComponent`)와 소음 발생 애님 노티파이

**경계 (비담당)**
- AIController·Blackboard SET 진입점·락온 대상 선정은 안 한다 — 컨트롤러는 `Source/WxGame/Controller/WxAIController.h`(AWxAIController)가, 겨눌 대상은 [[WxCombat]]의 `UWxLockOnComponent`가 소유한다. 이 모듈은 그 대상을 어떻게 바라볼지만 정한다.
- 전투 규칙(GameplayEffect·AttributeSet)을 참조하지 않는다 — WxCombat에 의존하지 않는 것이 설계. 감속 이펙트·속성 키는 디자이너가 BT 에디터에서 직접 지정한다. [[WxCombat]]
- BT 애셋·Blackboard 애셋 자체는 데이터로, 이 모듈은 그것을 소비하는 C++ 노드만 제공한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 키 이름+타입 accessor 네임스페이스. 키 SET/CLEAR 소유 분담(컨트롤러 vs 노드)을 헤더 주석이 지도로 정리 — 데이터 흐름 시작점 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIBehaviorComponent` | 캐릭터에 붙어 BT·감각·피격 자극을 캐릭터에 따라다니게 함. 컨트롤러 빙의 시점에 감각 수치를 퍼셉션에 밀어 넣음 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `UWxBTService_UpdateTargetActor` | 퍼셉션 감지 → Blackboard TargetActor 발행. 적 감지 파이프라인의 입구 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | TargetActor를 컨트롤러 포커스+폰 strafe 회전 모드로 반영. AI판 락온(플레이어 락온과 별개) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | GAS 어빌리티를 태그로 발동하고 종료까지 latent로 대기 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 조건 통과 자식 중 무작위 1개 실행. `RandomWeight` 데코로 가중, Selector 폴백 없음 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_Patrol` | `UBTTask_MoveTo` 상속, 도착 시 `UWxPatrolComponent`에 정찰 커서 진행 위임 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxPatrolComponent` | 스플라인 포인트를 정찰 지점으로 제공하는 무상태 경로 데이터(커서는 BT 태스크가 폰별 소유) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`·`UBTService`·`UBTDecorator`·`UBTCompositeNode`)를 상속하고 `WXAI_API`로 노출한다. 인스턴스별 상태가 필요하면 `GetInstanceMemorySize`/`InitializeMemory`로 노드 메모리에 실거나 `bCreateNodeInstance`를 쓴다(`WxBTTask_Patrol` 참고).
- Blackboard 값은 `GetValueAs`/`SetValueAs` 직접 호출 대신 `WxBlackboardKeys`의 키별 accessor를 통한다(타입 오용·조용한 기본값 반환을 경고 로그로 드러냄). 새 키는 accessor를 추가하고 Blackboard 애셋에 동명 키를 등록해야 한다.
- 데이터 주도: WxCombat에 의존하지 않으므로 전투 연동은 BT 에디터에서 디자이너가 지정한다 — 정찰 감속은 `MoveSpeedEffect`(`WxEffect_MoveSpeedScale` 류 GameplayEffect), 속성 게이팅은 `AttributeRatio` 데코의 `Attribute`/`MaxAttribute`(예: `WxCombatAttributeSet::HP`), 어빌리티는 태그(`Ability.*`)로 지목한다.
- 권한: 소음 보고(`UWxAnimNotify_ReportNoise` → `UAISense_Hearing::ReportNoiseEvent`)와 퍼셉션은 서버 전용. 피격 자극은 폰이 컨트롤러에 빙의돼 있을 때(=AI 조종 중)만 리스너가 있어 유효하다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 키 소유 분담 주석이 이 모듈 전체의 데이터 흐름 지도다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` — 캐릭터가 AI로 구동되기 시작하는 지점(BT·감각·자극 배선).
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` + `WxBTService_LockOn.h` — 감지 → TargetActor → 조준으로 이어지는 파이프라인.

## 관련
- 상위: `Source/WxGame/Controller/WxAIController.h`(AWxAIController)가 이 모듈의 BT 노드·Blackboard 키를 구동하고 SelfActor·HomeLocation·Master를 SET 한다. 락온 대상 선정은 [[WxCombat]] `UWxLockOnComponent`가 담당한다.

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 34파일 — `/readme-writer`로 갱신*
