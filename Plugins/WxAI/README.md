# WxAI — AI 시스템

> 적 AI의 인지(Perception)와 의사결정(Behavior Tree)을 담당한다. 엔진 AIModule 위에 프로젝트 규약(Blackboard 키, 리시 복귀, 어빌리티 발동, 정찰/배회)을 얹은 노드·컴포넌트 모음이다.

## 책임
**담당**
- Perception 셋업(Sight/Hearing/Damage)과 감지 결과의 TargetActor 동기화, 전투 인식(State.InCombat) 발행
- Blackboard 키 이름·값 타입 규약과 타입 안전 accessor (`WxBlackboardKeys`)
- 커스텀 BT 노드: 어빌리티 발동, 정찰/배회 이동, 리시 이탈 판정·복귀, 어트리뷰트 비율 조건, 가중 랜덤 선택, 타겟 거리 서비스
- 정찰 경로 데이터(스플라인)와 순회 규칙(`UWxPatrolComponent`)
- 팀 구분 enum(`EWxTeam`), 애님 노티파이 기반 소음 발생

**경계 (비담당)**
- 어빌리티·이펙트·어트리뷰트의 정의와 실행은 하지 않는다 — BT 노드는 GameplayTag/`FGameplayAttribute`/`TSubclassOf<UGameplayEffect>`를 디자이너가 BT 에디터에서 지정하도록 노출만 하며, 실체는 [[WxCombat]]가 소유(WxAI는 WxCombat에 의존하지 않음)
- Behavior Tree/Blackboard 에셋 자체, AIController·폰 클래스는 여기서 정의하지 않는다(콘텐츠·게임 모듈 담당)
- 적 스폰과 정찰 컴포넌트 배치는 `AWxSpawner`(외부)가 하고, 본 모듈은 `FindPatrolComponent`로 조회만 한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→TargetActor 동기화·인식 발행의 단일 지점, 리시 복귀 시 타겟 억제 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | BB 키 이름·타입 규약과 accessor(타 노드가 공유하는 계약면) | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTDecorator_BeyondLeash` | 앵커 키에서 LeashRadius 이탈 판정, 복귀 브랜치 게이팅(폴링 재평가) | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 리시 복귀 이동 + 퍼셉션 억제 진입/해제, 복귀 완료 판정 소유 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h` |
| `UWxBTComposite_RandomChoice` | 조건 통과 자식 중 가중 랜덤 1개 실행(폴백 없음) | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | GameplayTag로 어빌리티 발동, 종료 결과를 노드 결과로 매핑 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxPatrolComponent` | 정찰 경로(스플라인) 데이터·순회 규칙, 상태 없음(커서는 Task 소유) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTTask_Patrol` / `UWxBTTask_Wander` | 감속 GE를 실은 정찰/배회 이동 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |

## 확장 포인트 / 규약
- **Blackboard 계약**: 모든 노드는 `WxBlackboardKeys`의 고정 키(`SelfActor`/`TargetActor`/`HomeLocation`/`PatrolTargetLocation`/`TargetDistance`)로 통신한다. Blackboard 에셋에 같은 이름·타입의 키가 등록돼 있어야 하며, 새 노드는 `GetValueAs`/`SetValueAs` 직접 호출 대신 이 accessor를 쓴다.
- **WxCombat 비의존 규약**: 전투 관련 실체(어빌리티·GE·어트리뷰트)는 코드 의존 대신 BT 에디터 노출 프로퍼티로 주입한다. 새 노드도 이 방식을 따라 WxAI→WxCombat 방향 의존을 만들지 않는다.
- **리시(leash) 모델**: `BeyondLeash`(앵커 키 기준 이탈 판정) + `ReturnHome`(억제 진입/해제)가 짝을 이룬다. 복귀 브랜치는 전투 브랜치보다 상위 우선순위에 배치해야 게이팅이 성립한다. 데코는 복귀가 도는 동안 조건을 참으로 유지해 완료 판정을 Task 에 넘기므로, FlowAbortMode 를 무엇으로 두든 복귀가 경계에서 끊기지 않는다(None 이면 폴링 없이 탐색 시점에만 걸린다).
- **가중 랜덤**: `RandomWeight` Decorator는 조건이 아닌 데이터 운반자(항상 true)로, 자식에 붙여 `RandomChoice`의 추첨 가중치를 준다. 없으면 1.0, 0이면 제외.
- **노드 인스턴스 상태**: 정찰 커서·PingPong 방향 등 폰별 상태는 노드 인스턴스에 보관해 경로 공유·리스폰에 안전하다. 커스텀 Composite/Decorator는 `FBTCompositeMemory` 뒤에 자체 메모리를 배치하는 패턴을 지킨다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 노드 간 통신 계약. 나머지 노드가 무엇을 읽고 쓰는지의 기준.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 확보/소실/억제/인식의 전체 수명. 감지 로직의 중심.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` + `WxBTTask_ReturnHome.h` — 리시 이탈·복귀가 어떻게 짝지어 동작하는지.

## 관련
- 상위: BT/Blackboard/AIController 에셋을 제작하는 콘텐츠 및 게임 모듈, 적을 배치·스폰하는 `AWxSpawner`가 본 모듈의 노드·컴포넌트를 소비한다.
- 함께 보기: [[WxCombat]] (어빌리티·GE·어트리뷰트 실체), [[WxCore]] (공용 정의·GameplayTag)

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 29파일 — `/readme-writer`로 갱신*
