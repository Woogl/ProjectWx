# WxAI — AI 시스템

> 적 폰의 행동을 Behavior Tree 로 굴린다. 퍼셉션 감지·타겟팅·정찰·리시 복귀·어빌리티 발동을 담당하는 커스텀 BT 노드와 폰별 AI 셋업 컴포넌트를 제공한다.

## 책임
**담당**
- 캐릭터에 붙는 AI 셋업(행동 자산·감각 수치·피격 자극 보고)과, 컨트롤러 빙의 시점에 폰별 퍼셉션 설정 주입
- 타겟 획득/거리/락온/미러링 등 Blackboard 상태를 굴리는 BT Service
- 어빌리티 발동·정찰·배회·리시 복귀 BT Task 와 이를 게이팅하는 Decorator·Composite
- 스플라인 기반 정찰 경로 데이터, Blackboard 키 이름/타입 accessor

**경계 (비담당)**
- 어그로 대상 선정·락온 대상 보관은 게임 모듈의 `AWxAIController`/`UWxLockOnComponent` 소유 (WxAI 는 그 대상을 "어떻게 바라볼지"만 정함)
- 어트리뷰트 정의는 [[WxCombat]] — `UWxBTDecorator_AttributeRatio` 는 어떤 어트리뷰트를 볼지 BT 에디터에서 디자이너가 지정

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIBehaviorComponent` | 캐릭터측 AI 셋업 진입점 — 폰을 따라다니며 빙의 시 퍼셉션 주입, 피격을 촉각 자극으로 보고 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `WxBlackboardKeys` | 키 이름/값 타입을 묶은 accessor 네임스페이스 — Service/Task/Controller 가 공유하는 계약 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTService_UpdateTargetActor` | 퍼셉션 감지 결과를 TargetActor 로 발행 (타겟 획득의 단일 출처) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | TargetActor 를 컨트롤러 포커스+폰 strafe 회전에 반영 (겨누는 방식의 단독 소유) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTComposite_RandomChoice` | 자식 무작위 1택 실행 — 조건 데코+`RandomWeight` 로 후보 필터링 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | GameplayTag 로 어빌리티를 발동하고 종료까지 대기 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTDecorator_BeyondLeash` | 리시 이탈을 폴링 판정해 복귀 브랜치를 게이팅 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터(순회 규칙 포함), 상태 없음 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`)를 상속하고 `WXAI_API` 로 노출한다. Blackboard 접근은 반드시 `WxBlackboardKeys` accessor 를 거쳐 키 이름/타입 오용을 막는다.
- 이동형 Task 는 `UBTTask_MoveTo` 를 상속해 경로 판정을 엔진에 맡기고 도착 처리만 오버라이드한다(`WxBTTask_Patrol`/`WxBTTask_ReturnHome` 참고).
- 인스턴스 상태가 필요한 노드는 전용 memory struct + `GetInstanceMemorySize`/`InitializeMemory` 로 폰별 상태를 보관한다(트리·경로 재사용 안전). Composite 파생은 memory struct 를 `FBTCompositeMemory` 뒤에 배치한다.
- 정찰 경로는 폰이 부착된 액터(스포너 등)에 `UWxPatrolComponent` 를 달면 `FindPatrolComponent` 가 자동으로 집는다.
- 어트리뷰트 비교/어빌리티 발동은 GameplayAbilities 를 쓰되 특정 AttributeSet 을 하드코딩하지 않고 디자이너가 태그/어트리뷰트로 주입한다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` — 폰이 AI 로 굴러가는 진입점. 퍼셉션 주입·자극 보고의 소유 관계를 먼저 잡는다.
2. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 Service/Task 가 공유하는 상태 계약. 키별 담당자와 sentinel(NoTargetDistance) 규약이 여기 있다.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` — 컨트롤러 포커스와 폰 회전 모드가 갈라지기 쉬운 상태를 한 서비스가 묶는 이유. 노드 간 소유권 분리 원칙의 대표 사례.

## 관련
- 상위: 게임 모듈의 `AWxAIController`(SelfActor·HomeLocation·Master 발행, 락온 대상 보관)가 이 모듈의 BT 자산을 구동한다. 어빌리티·어트리뷰트는 [[WxCombat]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 36파일 — `/readme-writer`로 갱신*
