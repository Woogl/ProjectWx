# WxAI — AI 시스템

> AI 폰의 판단 계층을 담당한다. 퍼셉션으로 잡은 자극을 Blackboard 타겟으로 정제하고, Behavior Tree 노드(Composite·Decorator·Service·Task)로 전투·정찰·소환수 행동을 조립한다.

## 책임
**담당**
- Blackboard 키 이름·타입의 단일 계약(`WxBlackboardKeys`)과 오용 진단
- 퍼셉션 감지 결과 → `TargetActor` 선정, `TargetDistance` 갱신 같은 "인지 → 블랙보드" 변환
- BT 노드 저작 팔레트: 무작위 분기, 리시(leash) 이탈 판정, 어트리뷰트 비율 조건, 정찰·배회·복귀 이동, 어빌리티 발동
- AI 락온(컨트롤러 포커스 + 폰 회전 모드)을 한 쌍으로 묶어 소유
- 소환수/분신 계열: Master ASC 구독과 어빌리티 미러링
- 폰에 붙는 AI 설정 보관(`UWxAIBehaviorComponent`: BT 에셋, 감각 수치, 정찰 경로, 피격 자극 보고)과 스플라인 정찰 경로 데이터(`UWxPatrolComponent`)

**경계 (비담당)**
- AIController 자체(퍼셉션 센스 구성, 팀 판정, `RunBehaviorTree`, 빙의 시 컨텍스트 키 세팅)는 `Source/WxGame/Controller/WxAIController.*`에 있다. 이 모듈은 그 컨트롤러가 호출하는 부품만 제공한다.
- 락온 "대상 보관"과 어트리뷰트/GameplayEffect 정의는 [[WxCombat]]. WxAI는 이 모듈을 참조하지 않으므로, 어트리뷰트·감속 이펙트는 C++ 하드코딩 대신 BT 에디터에서 디자이너가 지정하는 방식으로 연결한다.
- 공용 GameplayTag 정의는 [[WxCore]](`WxGameplayTags`). 이 모듈은 태그를 선언하지 않고 소비만 한다.
- Behavior Tree/Blackboard 에셋 저작, 스포너 배치는 데이터 쪽 몫이다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 모듈 전체가 공유하는 데이터 계약. 노드들은 서로 직접 대화하지 않고 전부 이 키를 거친다 — 여기부터 읽어야 노드 간 결합이 보인다 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIBehaviorComponent` | 폰 쪽 AI 설정 보관소이자 컨트롤러와의 접점. 컨트롤러가 빙의 시 여기서 BT 에셋을 받아 실행하고, 감각 수치는 반대로 이 컴포넌트가 컨트롤러에 밀어 넣는다 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `UWxBTService_UpdateTargetActor` | `TargetActor`의 유일한 발행자. 타겟 선정 규칙을 바꾸려면 이 노드만 본다 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | 위가 발행한 타겟의 소비자. 컨트롤러 포커스와 폰 회전 모드를 단독 소유해 두 시스템의 상태 경합을 막는다 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | BT → GAS 경계. 어빌리티 종료 통지를 BT 노드 결과로 번역한다 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 무작위 행동 선택의 축. `UWxBTDecorator_RandomWeight`와 반드시 한 쌍으로 읽는다(데코가 조건이 아니라 가중치 운반자다) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxPatrolComponent` | 상태 없는 경로 데이터. 진행 커서는 `UWxBTTask_Patrol`이 폰별로 들고 있어 경로 공유·리스폰에 안전하다 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTTask_MirrorAbility` | 소환수/분신 계열의 중심. `UWxBTService_ObserveMasterAbility`·`UWxBTDecorator_MasterAbility`·`UWxBTService_MirrorMovement`가 같은 `Master` 키를 두고 함께 돈다 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h` |

## 확장 포인트 / 규약
- **새 BT 노드 추가**: 엔진 베이스(`UBTTaskNode` / `UBTService` / `UBTDecorator` / `UBTCompositeNode`)를 그대로 상속하고 `Public/`에 `WxBT<종류>_<이름>.h`로 둔다. Blackboard 접근은 `GetValueAsX`를 직접 부르지 말고 `WxBlackboardKeys`의 accessor를 쓴다(키 부재·타입 불일치를 경고로 드러낸다). 새 키가 필요하면 `WxBlackboardKeys.h`/`.cpp` 양쪽에 accessor를 추가하고, Blackboard 에셋에도 같은 이름으로 등록해야 한다.
- **키 소유권**: `SelfActor`·`HomeLocation`·`Master`는 컨트롤러가(빙의/해제 시), `TargetActor`·`TargetDistance`·`PatrolTargetLocation`은 BT 노드가 쓴다. `TargetDistance`는 타겟 부재 시 0이 아니라 `NoTargetDistance`가 들어간다 — 근거리 비교 조건을 짤 때 전제가 된다.
- **WxCombat 비의존을 데이터로 메우는 지점**: `UWxBTDecorator_AttributeRatio`의 `Attribute`/`MaxAttribute`, `UWxBTTask_Patrol`·`UWxBTTask_Wander`의 `MoveSpeedEffect`는 C++에서 타입을 알 수 없어 BT 에디터에서 지정한다. 미지정 시 조용히 기본 동작(감속 없음)으로 떨어지므로 저작 누락에 주의한다.
- **정찰 배선**: 경로는 폰이 아니라 **스포너**에 붙인다. `UWxAIBehaviorComponent::InitializeComponent`가 빙의로 Owner가 덮이기 전에 스폰 주체에서 `UWxPatrolComponent`를 찾아 두는 순서에 의존하며, 이 순서는 `Private/Tests/WxPatrolPathSpawnTest.cpp`가 실제 스폰·빙의로 지킨다.
- **권한 모델**: 퍼셉션·BT·소음 보고(`UWxAnimNotify_ReportNoise`)는 모두 서버 권한 경로에서만 의미를 가진다. 이 모듈은 자체 리플리케이션 상태를 두지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 노드들이 무엇을 주고받는지, 누가 쓰고 누가 읽는지가 여기에 다 있다.
2. `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp` — 폰 → 컨트롤러로 설정이 흘러가는 초기화 순서(빙의 타이밍 의존)를 확인한다.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` + `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` — 타겟 "발행"과 "소비"를 분리한 이 모듈의 핵심 설계.
4. `Source/WxGame/Controller/WxAIController.cpp` — 모듈 밖이지만, BT 실행과 컨텍스트 키 세팅이 실제로 일어나는 곳이라 함께 봐야 그림이 닫힌다.

## 관련
- 상위: `Source/WxGame` (AWxAIController, AWxEnemyCharacter가 이 모듈을 조립한다)
- 함께 보는 모듈: [[WxCore]] (GameplayTag 정의), [[WxCombat]] (락온 대상 보관·어트리뷰트·GameplayEffect — 코드 의존 없이 BT 저작으로만 연결)

---
*문서 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 41파일 — `/readme-writer`로 갱신*
