# WxAI — AI 시스템

> 적 폰의 감지·행동을 담당한다. AIPerception 을 Blackboard 타겟으로 동기화하고, 그 타겟을 소비하는 Behavior Tree 노드(Task·Service·Decorator·Composite)와 정찰 경로 데이터를 제공한다.

## 책임
**담당**
- 시각·청각·피격 감지를 Blackboard `TargetActor` 로 동기화 (`UWxAIPerceptionComponent`)
- Blackboard 키 이름·타입·accessor 계약 (`WxBlackboardKeys`)
- 전투 행동 BT 노드: 어빌리티 발동/미러링, 정찰·배회·복귀 이동, 무작위 선택, 리시·어트리뷰트 게이팅
- AI 락온(컨트롤러 포커스 + 폰 strafe 회전 모드)의 단독 소유 (`UWxBTService_LockOn`)
- 스플라인 기반 정찰 경로 데이터 (`UWxPatrolComponent`)

**경계 (비담당)**
- 겨눌 대상 자체의 발행·AIController 소유 → `AWxAIController`(WxGame 게임 모듈). 이 모듈은 그 대상을 어떻게 바라보고 소비할지만 정한다.
- 어트리뷰트·GameplayEffect·어빌리티 정의 → [[WxCombat]]. WxAI 는 이를 참조하지 않으며, 어트리뷰트/이펙트/어빌리티 태그는 디자이너가 BT 에디터에서 직접 지정한다.
- Blackboard/BehaviorTree 에셋 저작 (같은 이름의 키 등록은 에셋 쪽 책임)

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`·`UBTService`·`UBTDecorator`·`UBTCompositeNode`)를 상속하고 `Wx` 접두사를 붙인다. 이동 계열은 `UBTTask_MoveTo` 를 상속해 이동/도착 판정을 엔진에 맡긴다(`WxBTTask_Patrol`·`WxBTTask_ReturnHome`).
- Blackboard 접근은 `GetValueAs`/`SetValueAs` 직접 호출 대신 `WxBlackboardKeys` 의 키별 accessor 를 쓴다 — 키 이름·값 타입을 한 곳에 묶어 타입 오용을 막고, 잘못된 접근을 경고 로그로 드러낸다.
- 데이터 주도 설정은 노드의 `UPROPERTY(EditAnywhere)` 로 노출한다. WxCombat 을 참조할 수 없으므로 어트리뷰트·이펙트·어빌리티 태그는 셀렉터/`TSubclassOf`/`FGameplayTag` 로 열어 두고 저작 값에 맡긴다.
- 노드 상태는 `bCreateNodeInstance` 또는 노드 메모리 구조체(`FWx...Memory`)에 폰별로 보관한다. Composite 계열은 베이스가 쓰는 `FBTCompositeMemory` 뒤에 자체 상태를 배치한다.
- 권한: 소음 발생(`UWxAnimNotify_ReportNoise`)은 서버 전용이며, AI 로직은 서버 권한 폰에서 돈다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지 → `TargetActor` 발행의 유일한 소스. 사망·파괴·복귀에서만 해제 | `Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | Blackboard 키 이름·타입·accessor 계약. 폰-BT 간 데이터 규약의 중심 | `Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTService_LockOn` | `TargetActor` 를 컨트롤러 포커스 + 폰 회전 모드에 반영. 락온 상태 단독 소유 | `Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | `AbilityTag` 로 어빌리티 발동, 종료까지 latent 대기 | `Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_MirrorAbility` | 지목 대상이 쓰는 어빌리티를 같은 태그로 따라 발동/해제 | `Source/WxAI/Public/WxBTTask_MirrorAbility.h` |
| `UWxBTDecorator_BeyondLeash` + `UWxBTTask_ReturnHome` | 리시 이탈 판정(폴링) 과 홈 복귀 이동. 복귀 완료는 Task 가 단독 판정 | `Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTComposite_RandomChoice` + `UWxBTDecorator_RandomWeight` | 조건 통과 자식 중 가중치 무작위 1개 선택 | `Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터(무상태). 커서는 `UWxBTTask_Patrol` 이 폰별 소유 | `Source/WxAI/Public/WxPatrolComponent.h` |

## 여기서부터 읽어라
1. `Source/WxAI/Public/WxBlackboardKeys.h` — 폰·퍼셉션·BT 노드가 어떤 키를 나눠 쓰는지가 모듈 전체의 데이터 흐름 지도다.
2. `Source/WxAI/Public/WxAIPerceptionComponent.h` — `TargetActor` 가 언제 발행/해제되는지. 대부분의 전투 브랜치가 이 키에서 시작한다.
3. `Source/WxAI/Public/WxBTService_LockOn.h` — 퍼셉션(감지)과 락온(응시)의 책임 경계, 그리고 AIController 와의 소유 분리를 설명한다.

## 관련
- 상위: [[WxCore]] (공용 정의), `AWxAIController`(WxGame)
- 저작 값 연계: [[WxCombat]] (어트리뷰트·이펙트·어빌리티, 직접 의존 없음)

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 32파일 — `/readme-writer`로 갱신*
