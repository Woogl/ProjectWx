# WxAI — AI 시스템

> 적 NPC의 지각(Perception)·행동트리(BehaviorTree) 실행·정찰 경로를 담당한다. 엔진 AIModule 위에 커스텀 BT 노드와 퍼셉션/블랙보드 규약을 얹어, 감지 → 추격 → 전투 → 리시 복귀의 AI 루프를 구성한다.

## 책임
**담당**
- AIController에 붙는 지각 컴포넌트(`UWxAIPerceptionComponent`): Sight/Hearing/Damage 감지를 Blackboard `TargetActor`에 동기화하고, 타겟 유무에 따라 회전 모드와 `State.InCombat` 인식 태그를 발행한다.
- 커스텀 BT 노드 팔레트: Task(어빌리티 발동/정찰/배회/복귀), Service(타겟 거리 기록), Decorator(리시 이탈·어트리뷰트 비율·랜덤 가중치), Composite(가중 랜덤 선택).
- 정찰 경로 데이터(`UWxPatrolComponent`, 스플라인 기반)와 Blackboard 키 이름·타입드 accessor 규약(`WxBlackboardKeys`).
- 소음 이벤트 발생 AnimNotify(`UWxAnimNotify_ReportNoise`)로 청각 감지 자극 공급.

**경계 (비담당)**
- 어빌리티 정의·어트리뷰트 셋 자체는 [[WxCombat]]에 있다. WxAI는 GAS ASC를 발동·조회만 하며, `WxBTDecorator_AttributeRatio`가 참조할 Attribute는 디자이너가 BT 에디터에서 지정한다(코드 의존 없음).
- 스폰·스포너(`AWxSpawner`)는 이 모듈이 아니며, 정찰 컴포넌트는 스포너가 Owner로 지정한 액터에서 조회된다.
- 리시 이탈 "판정 로직"은 BT(Decorator+Task)로 이관되어 있고, 지각 컴포넌트는 억제 지시만 받는다.

## 의존성
- **주요 의존**: `WxCore`, `AIModule`, `GameplayAbilities`(GAS ASC 발동/조회), `GameplayTags`, `GameplayTasks`, `NavigationSystem`.
- 규칙: WxCore 외 다른 Wx 플러그인 참조 — 없음 ✅ (uplugin는 `GameplayAbilities`, `WxCore`만 활성화)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 지각 → `TargetActor`/인식/회전 동기화의 단일 지점, 타겟 소실·억제 처리 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | BT ↔ 지각이 공유하는 Blackboard 키 이름·타입드 accessor 규약(namespace) | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터 + MoveMode 순회 규칙(상태 없음) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTTask_ActivateAbility` | `AbilityTag`로 GAS 어빌리티를 발동하고 종료를 기다리는 BT Task | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 유효 후보 중 가중 랜덤 1개 선택(폴백 없음), `RandomWeight` Decorator 소비 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTDecorator_BeyondLeash` | 홈 반경 이탈을 매 틱 폴링해 복귀 브랜치를 게이팅(실시간 abort) | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 복귀 진입 시 지각 억제 지시, 홈으로 MoveTo | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h` |

## 확장 포인트 / 규약
- **새 BT 노드**: 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`)를 상속해 `Wx` prefix로 추가한다. Blackboard 접근은 반드시 `WxBlackboardKeys` accessor를 거쳐 키 이름·값 타입 오용을 막는다.
- **Blackboard 키 추가**: `WxBlackboardKeys` namespace에 `FName`과 타입드 accessor를 함께 선언·정의하고, 대상 Blackboard 에셋에 동일 이름·타입 키를 등록한다(불일치 시 accessor가 경고 로그).
- **리시(leash) 브랜치 배치**: `BeyondLeash` Decorator의 `FlowAbortMode`를 반드시 `LowerPriority`로 지정(Self/Both 금지). 복귀 브랜치가 전투 브랜치보다 상위 우선순위여야 실시간 abort가 성립한다.
- **가중 랜덤 패턴**: `RandomChoice` Composite 아래 자식에 `RandomWeight` Decorator로 가중치를, `AttributeRatio` 등 조건 Decorator로 후보 필터를 건다(Weight 0 또는 조건 실패 시 추첨 제외).
- **정찰**: 정찰 진행 커서는 `UWxBTTask_Patrol`이 폰별로 소유하고, 경로 데이터는 `UWxPatrolComponent`가 무상태로 제공해 다중 폰·리스폰에 안전하다.
- **인식 태그**: `State.InCombat`은 서버에서 폰 ASC에 MinimalReplication으로 발행되어 네임플레이트가 소비한다(C++ Native Tag 선언은 없음).

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — BT와 지각이 공유하는 데이터 계약. 이 키들을 알아야 나머지 노드가 읽힌다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 감지→타겟→인식의 상태 흐름과 타겟 소실·억제 규칙(헤더 doc-comment가 설계 근거를 담음).
3. `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` + `WxBTTask_ReturnHome.h` — 리시 이탈 판정이 BT로 이관된 이유와 지각의 억제 연계.

## 관련
- 상위: 적 캐릭터/스포너와 BehaviorTree·Blackboard 에셋이 이 노드들을 조립해 사용. GAS 어빌리티·어트리뷰트는 [[WxCombat]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 29파일 — `/readme-writer`로 갱신*
