# WxAI — AI 시스템

> 적 NPC의 감지·판단·행동을 구동한다. 언리얼 Behavior Tree 위에 얹는 커스텀 노드 묶음과, 그 노드들이 공유하는 Perception·Blackboard·정찰 경로 인프라를 제공한다.

## 책임
**담당**
- 감지 → 타겟 결정: 시각·청각·피격 자극을 하나의 `TargetActor`로 정리(`UWxAIPerceptionComponent`)
- Blackboard 키 규약: 키 이름·타입·accessor를 한 곳에 묶어 소유자별 SET/CLEAR를 정리(`WxBlackboardKeys`)
- BT 커스텀 노드: 정찰/배회/복귀/어빌리티 발동 Task, 락온/거리 갱신 Service, 리시·어트리뷰트·가중치 Decorator, 무작위 선택 Composite
- 정찰 경로 데이터: 스플라인 기반 순수 경로 + 순회 규칙(`UWxPatrolComponent`)
- 락온 상태 소유: 컨트롤러 포커스 + 폰 회전 모드를 한 쌍으로 관리(`UWxBTService_LockOn`)

**경계 (비담당)**
- `AIController`·Blackboard 에셋·BT 에셋 저작: 이 모듈은 노드/컴포넌트만 제공하고, `SelfActor`·`HomeLocation`·`Master` 키를 쓰는 컨트롤러와 트리 구성은 게임 측([[WxGame]])에 있다.
- 전투 로직·어트리뷰트·GameplayEffect 정의: 발동만 여기서 하고 실체는 [[WxCombat]]. 감속 효과(`WxEffect_MoveSpeedScale`)·비교 어트리뷰트는 디자이너가 BT 에디터에서 지정한다(런타임 의존 없음).
- 플레이어 락온: 이름만 같은 별개 구현(`UWxLockOnComponent`, [[WxCombat]]).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 자극 → `TargetActor` 동기화. 데이터 흐름의 시작점 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | 모든 노드가 공유하는 Blackboard 키·accessor 규약 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTService_LockOn` | 포커스+회전 모드 소유. 퍼셉션과 역할 분담의 경계 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTComposite_RandomChoice` | 조건·가중치 반영 무작위 분기(짝: `RandomWeight`) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | BT ↔ GAS 다리. 어빌리티 발동·종료를 Task 결과로 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_Patrol` | `MoveTo` 상속 정찰 실행. 커서를 폰별로 소유 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxBTDecorator_BeyondLeash` | 리시 이탈 게이팅. `ReturnHome` Task와 한 쌍 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로(상태 없는 순수 데이터) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 Blackboard 키를 늘릴 때는 `WxBlackboardKeys`에 이름·accessor를 함께 추가한다 — 키 이름과 값 타입을 한 곳에 묶어 타입 오용을 막는 것이 이 네임스페이스의 목적이다. Blackboard 에셋에 같은 이름 키가 등록돼 있어야 한다.
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`)를 상속하고 `Wx|AI` 카테고리에 UPROPERTY를 노출한다. 노드 인스턴스별 상태(정찰 커서·락온 기록 등)는 `bCreateNodeInstance` 또는 노드 메모리 구조체로 폰별 격리한다.
- WxCombat 비의존 원칙: 감속·어트리뷰트·어빌리티는 코드 참조가 아니라 `TSubclassOf<UGameplayEffect>`/`FGameplayAttribute`/`FGameplayTag`(`Ability` 계열) UPROPERTY로 받아 BT 에디터에서 주입한다. 미지정이면 해당 효과 없이 동작한다.
- 정찰 경로는 적이 부착된 액터(스포너 등)에 `UWxPatrolComponent`를 붙여 구동한다. 경로는 데이터, 진행 커서는 Task 소유 — 같은 경로를 여러 폰이 공유해도 안전하다.
- 소음은 서버 전용이며 실제 청취 거리는 `min(HearingDistance, 청취자 HearingRange)`다(`UWxAnimNotify_ReportNoise`).

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 노드가 공유하는 상태 계약. 어떤 키를 누가 쓰고 지우는지가 시스템 전체의 지도다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 데이터 흐름의 입구. 자극이 `TargetActor`로 바뀌는 규칙과 타겟 소실 처리.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` — 퍼셉션과 BT의 역할 경계가 가장 명확히 드러나는 지점(감지 vs 응시).
4. `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp` — 조건·가중치 필터링과 재추첨 로직. 커스텀 Composite 동작의 핵심.

## 관련
- 상위: BT/Blackboard/`AIController` 에셋을 저작해 이 노드들을 배치하는 게임 콘텐츠 및 [[WxGame]]
- 함께: 어빌리티·어트리뷰트·GameplayEffect 실체를 제공하는 [[WxCombat]], 공용 정의의 [[WxCore]]

---
*문서 기준 커밋 `f0aad4c` · 생성일 2026-09-03 · 소스 30파일 — `/readme-writer`로 갱신*
