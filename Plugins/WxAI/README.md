# WxAI — AI 시스템

> 적 폰의 감지·판단·행동을 담당한다. AIPerception으로 타겟을 잡아 Blackboard에 실어 두고, Behavior Tree용 커스텀 Task·Service·Decorator·Composite 노드와 정찰 경로로 그 타겟에 반응하는 행동을 조립한다.

## 책임
**담당**
- 퍼셉션: 시각·청각·피격 감지를 Blackboard `TargetActor`로 동기화 (`UWxAIPerceptionComponent`)
- BT 노드 라이브러리: 어빌리티 발동·미러링, 정찰·복귀·배회 이동, 락온·거리 서비스, 리시·비율·가중치 데코레이터, 무작위 선택 Composite
- Blackboard 키 규약: 키 이름·타입·accessor를 한곳에 묶어 오용 방지 (`WxBlackboardKeys`)
- 정찰 경로 데이터: 스플라인 기반 순수 경로 + 순회 규칙 (`UWxPatrolComponent`)
- 소음 유발 AnimNotify (`UWxAnimNotify_ReportNoise`)

**경계 (비담당)**
- 어빌리티 실체·GAS 실행: BT 노드는 태그로 발동만 걸고 실제 어빌리티는 [[WxCombat]] 등 저작 애셋에 위임
- 정찰 감속 GameplayEffect(`WxEffect_MoveSpeedScale`)도 WxAI가 코드 의존하지 않고 BT 에디터에서 디자이너가 지정
- 플레이어 락온 구현은 별개 (AI 락온만 `UWxBTService_LockOn`이 담당)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→`TargetActor` 발행. 타겟 획득/소실의 단일 소유자 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | 폰·컨트롤러·BT 노드가 공유하는 키 규약. 데이터 흐름의 허브 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTService_LockOn` | `TargetActor`를 컨트롤러 포커스+폰 회전 모드에 반영 (조준의 단일 소유자) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | 태그로 어빌리티 발동, 종료까지 latent 유지 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_MirrorAbility` | 대상이 쓰는 어빌리티를 같은 태그로 따라 발동/해제 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h` |
| `UWxBTTask_Patrol` | `MoveTo` 상속, 도착 시 정찰 커서 진행 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커 이탈(리시) 판정, 폴링으로 재평가 촉발 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로. 상태 없는 경로 데이터 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- **새 BT 노드**: 엔진 `UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`를 상속하고 `Wx` 접두사. 노드별 상태는 `bCreateNodeInstance` 또는 인스턴스 메모리 구조체(`FWx...Memory`)로 폰별 보관 — 같은 경로/트리를 여러 폰이 재사용하고 리스폰해도 안전하게.
- **Blackboard 접근은 `WxBlackboardKeys` accessor로만**. 직접 `GetValueAs`/`SetValueAs` 대신 accessor를 써 타입 오용을 막고, Blackboard 애셋에 같은 이름 키가 등록돼 있어야 한다. `TargetDistance`는 타겟 부재 시 `NoTargetDistance`(무한대)를 기록하는 규약.
- **데이터 주도 설정**: 어빌리티는 `FGameplayTag`/`FGameplayTagContainer`(`meta=(Categories="Ability")`)로, 이동 목표·앵커는 `FBlackboardKeySelector`로, 감속 GE는 `TSubclassOf<UGameplayEffect>`로 BT 에디터에서 지정.
- **상태 소유 분리**: 퍼셉션은 `TargetActor` 발행까지만, 조준(포커스+회전)은 `UWxBTService_LockOn`만, 리시 복귀 완료 판정은 `UWxBTTask_ReturnHome`만 — 같은 상태를 두 시스템이 다투지 않게 각 소유자를 단일화.
- **서버 권한**: `UWxAnimNotify_ReportNoise`의 소음 발생은 서버 전용.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 노드가 공유하는 데이터 계약. 누가 무엇을 SET/CLEAR하는지 여기서 파악.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟이 어떻게 잡히고 언제 풀리는지, 감지 흐름의 출발점.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` — 발행된 타겟이 실제 행동(조준)으로 이어지는 소비 지점. 상태 소유 분리 사고방식의 대표 예.

## 관련
- 상위: 적 폰의 `AIController`(예: `AWxAIController`)가 이 모듈의 컴포넌트·BT 노드를 조립해 사용. 어빌리티 태그·GE 애셋은 [[WxCombat]]과 함께 본다.
- 의존: [[WxCore]]

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 32파일 — `/readme-writer`로 갱신*
