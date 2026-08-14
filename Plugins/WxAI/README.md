# WxAI — AI 시스템

> 적 폰의 Behavior Tree 실행 요소(Task·Decorator·Service·Composite), 감각 인식, 그리고 이들이 공유하는 Blackboard 규약을 제공한다. AI의 "판단 트리"는 데이터(BT 에셋)로 두고, WxAI는 그 트리를 구성하는 C++ 노드와 인식·정찰 컴포넌트를 담는다.

## 책임
**담당**
- 감각 인식: Sight/Hearing/Damage 감지 → Blackboard `TargetActor` 동기화, 인식 상태(`State.InCombat`) 발행, 회전 모드(strafe/평상시) 전환 (`UWxAIPerceptionComponent`)
- 리시(leash) 모델: 배치 지점 이탈 판정 → 복귀 브랜치 게이팅 → 복귀 중 재감지 억제 (`UWxBTDecorator_BeyondLeash` + `UWxBTTask_ReturnHome`)
- 이동 계열 BT Task: 정찰(`UWxBTTask_Patrol`), 배회(`UWxBTTask_Wander`), 복귀(`UWxBTTask_ReturnHome`)와 정찰 경로 데이터(`UWxPatrolComponent`)
- 선택/가중치 제어: 가중 무작위 Composite(`UWxBTComposite_RandomChoice`) + 운반용 Decorator(`UWxBTDecorator_RandomWeight`)
- GAS 연동 Task: BT에서 어트리뷰트 비율 조건 판정(`UWxBTDecorator_AttributeRatio`)과 어빌리티 발동(`UWxBTTask_ActivateAbility`)
- Blackboard 키 이름·타입을 한곳에 묶은 타입 안전 accessor (`WxBlackboardKeys`)

**경계 (비담당)**
- 전투 로직·어트리뷰트·GameplayEffect 정의는 하지 않는다. 어빌리티/어트리뷰트/감속 Effect는 디자이너가 BT 에디터에서 태그·에셋으로 주입하며, 정의는 [[WxCombat]] 소관이다.
- BT/Blackboard 에셋 자체와 노드 배치·우선순위 설계는 데이터(BP 에셋) 몫이다. WxAI는 노드 클래스만 제공한다.
- AIController·폰 클래스·스폰 배치(`AWxSpawner`)는 이 모듈에 없다. 여기 컴포넌트/노드를 소비하는 쪽이다.

## 의존성
- **주요 의존**: `WxCore`(Gameplay 태그 `WxGameplayTags`), `GameplayAbilities`(ASC·어트리뷰트), `AIModule`·`GameplayTasks`(BT·Perception 기반), `NavigationSystem`(이동 Task), `GameplayTags`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (전투 의존은 코드가 아니라 BT 에디터의 태그·에셋 주입으로 끊어 둠)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→타겟·인식·회전모드를 관장하는 상태 머신. 타겟 수명(사망/파괴/억제) 관리의 중심 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | 인식 컴포넌트·BT 노드·AIController가 공유하는 BB 키의 단일 정의처(accessor 네임스페이스) | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTDecorator_BeyondLeash` | 리시 이탈을 매 프레임 폴링해 복귀 브랜치를 여는 게이트. FlowAbortMode 규약 주의 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 리시 복귀 실행 노드. 진입 시 퍼셉션에 타겟 억제 지시, 종료 시 해제 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ReturnHome.h` |
| `UWxBTComposite_RandomChoice` | 가중 무작위로 자식 1개만 실행(Selector 폴백 없음). RandomWeight를 룰렛에 소비 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | BT에서 `AbilityTag`로 ASC 어빌리티 발동, 종료 결과를 노드 결과로 변환 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로(상태 없는 순수 데이터). BT Task가 커서를 소유 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxAnimNotify_ReportNoise` | 애님 프레임에서 청각 소음 자극 발생(서버 전용) → 주변 AI 감지 | `Plugins/WxAI/Source/WxAI/Public/WxAnimNotify_ReportNoise.h` |

## 확장 포인트 / 규약
- **새 BT 노드 추가**: 엔진 베이스(`UBTTaskNode`/`UBTDecorator`/`UBTService`/`UBTCompositeNode` 또는 그 특화)를 상속해 `Public/`에 둔다. 이동형 Task는 `UBTTask_MoveTo`를 상속해 이동/도착/경로 실패를 엔진에 맡기고 도착 훅만 오버라이드하는 것이 이 모듈의 관례(`UWxBTTask_Patrol`).
- **Blackboard 접근은 반드시 `WxBlackboardKeys`를 경유**: 키 이름·값 타입이 한곳에 묶여 GetValueAs/SetValueAs 오용을 막는다. Blackboard 에셋에 동일 이름 키가 등록돼 있어야 하며, Object 키는 nullptr Set이 Clear와 동치, Float(`TargetDistance`)만 별도 Clear를 둔다.
- **전투 의존의 데이터 주도 우회**: WxAI는 WxCombat에 코드 의존하지 않으므로, 어트리뷰트(`UWxBTDecorator_AttributeRatio`의 Attribute/MaxAttribute), 발동 어빌리티 태그(`UWxBTTask_ActivateAbility`), 감속 GameplayEffect(`UWxBTTask_Patrol`/`_Wander`의 `WxEffect_MoveSpeedScale`)는 모두 BT 에디터에서 주입한다.
- **리시 브랜치 배치 규약**: `UWxBTDecorator_BeyondLeash`의 FlowAbortMode는 반드시 **Lower Priority**로, 복귀 브랜치는 전투 브랜치보다 상위 우선순위에 둔다. Self/Both는 경계 왕복을 유발해 금지.
- **권한 모델**: 인식/타겟팅 판정과 태그 발행은 서버에서 일어나며, `State.InCombat`은 폰 ASC에 MinimalReplication 태그로 발행되어 클라이언트 네임플레이트에 소비된다. 소음 발생(`UWxAnimNotify_ReportNoise`)도 서버 전용.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 확정·소실·억제·인식 발행의 전체 수명 모델이 클래스 주석에 정리돼 있다. 모듈의 제어 중심.
2. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 어떤 주체가 어떤 키를 쓰고 지우는지의 지도. BT 노드를 읽기 전 먼저 본다.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` — 리시 복귀가 퍼셉션·복귀 Task와 어떻게 맞물리는지, 왜 폴링/FlowAbortMode 제약이 있는지.

## 관련
- 상위: AIController·폰·스폰 배치(`AWxSpawner`)가 이 컴포넌트·노드를 조립해 소비한다. 전투 어트리뷰트/어빌리티/Effect 정의는 [[WxCombat]], 공용 태그(`WxGameplayTags`)는 [[WxCore]].

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 29파일 — `/readme-writer`로 갱신*
