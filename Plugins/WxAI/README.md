# WxAI — AI 행동 시스템

> 적 폰의 인지(시각·청각·피격)와 행동 트리 실행을 책임진다. 퍼셉션 → Blackboard → BT 노드로 이어지는 데이터 흐름을 제공하고, 정찰·배회·리시 복귀·타겟 락온·어빌리티/이동 미러 같은 재사용 BT 노드를 낸다.

## 책임
**담당**
- 폰의 감지·인식: 시각/청각/피격 센스 구성과 자극 수집 (`UWxAIPerceptionComponent`)
- 감지 결과를 판단 재료로 정리해 Blackboard 에 발행: 타겟 선정, 거리, 락온 (`WxBTService_*`)
- BT 실행 노드 라이브러리: 정찰/배회/복귀/어빌리티 발동·미러/이동 미러/무작위 선택·가중치/리시·어트리뷰트 게이트
- 정찰 경로 데이터(스플라인)와 순회 규칙 (`UWxPatrolComponent`)
- 폰별 감각 수치를 캐릭터 상속과 분리해 컨트롤러에 넘김 (`UWxAIBehaviorComponent`)

**경계 (비담당)**
- AIController 본체·폰 빙의·BehaviorTree/Blackboard 에셋 실행 시작 → 게임 모듈 `WxGame`(`AWxAIController`)
- 어빌리티 정의·어트리뷰트·이동속도 감속 이펙트 → [[WxCombat]] (BT 에디터에서 디자이너가 태그/에셋으로 지정, 코드 의존 없음)
- 플레이어 락온 및 겨누는 대상 산출 → [[WxCombat]] (`UWxLockOnComponent`). 이 모듈의 LockOn 서비스는 "어떻게 바라볼지"만 정함

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 컨트롤러에 붙어 시각·청각·피격 감지. 감지까지가 범위 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxAIBehaviorComponent` | 폰이 소유. BT 에셋·감각 수치를 컨트롤러에 공급 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `WxBlackboardKeys` | BT ↔ 컨트롤러 공유 키의 타입-세이프 accessor 네임스페이스 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTService_UpdateTargetActor` | 감지 액터 중 하나를 `TargetActor` 로 선정(타겟 두뇌). 루트에 상주 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | `TargetActor` 를 컨트롤러 포커스+폰 strafe 회전에 반영 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | 태그로 어빌리티를 발동하고 종료까지 대기 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 조건·가중치로 자식 1개 무작위 실행(Selector 폴백 없음) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터. 상태 없음(커서는 BT 태스크가 소유) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 `UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`(또는 그 파생)를 상속. 폰별 상태는 노드 인스턴스 메모리(`GetInstanceMemorySize`/`InitializeMemory`) 또는 `bCreateNodeInstance` 로 보관해 경로 공유·리스폰에 안전하게 한다.
- Blackboard 키는 `WxBlackboardKeys` accessor 로만 읽고 쓴다(직접 `GetValueAs`/`SetValueAs` 지양). Blackboard 에셋에 동명 키가 등록돼 있어야 한다. Object 키는 nullptr set = Clear, Float 거리는 타겟 부재 시 `NoTargetDistance`.
- 데이터 주도: 감각 수치·정찰 경로·BT 노드 프로퍼티는 컴포넌트/노드에서 편집. 어빌리티 태그, 어트리뷰트(`FGameplayAttribute`), 이동속도 감속 이펙트(`TSubclassOf<UGameplayEffect>`)는 WxCombat 자산을 BT 에디터에서 직접 지정한다.
- 리플리케이션: AI 판단·소음 보고(`UWxAnimNotify_ReportNoise`)는 서버 권위에서 돈다.
- 락온과 이동 미러는 둘 다 컨트롤러 포커스+회전 모드를 점유하므로 한 트리에 함께 두지 않는다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모듈 전체를 잇는 데이터 계약. 누가 어떤 키를 SET/CLEAR 하는지 여기서 잡힌다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 인지의 입구. 감지가 어떻게 들어오는지.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` — 감지에서 타겟 결정으로 넘어가는 지점.
4. `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` / `WxBTService_MirrorMovement.h` — 포커스·회전 소유권 규약(가장 얽히기 쉬운 부분).

## 관련
- 상위: `AWxAIController`(게임 모듈 `WxGame`)가 이 모듈의 컴포넌트를 붙이고 BT/Blackboard 를 구동한다.
- 함께: [[WxCombat]] — 어빌리티·어트리뷰트·이동속도 이펙트·`UWxLockOnComponent` 제공(코드 의존 아닌 에셋/태그 연동). 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 38파일 — `/readme-writer`로 갱신*
