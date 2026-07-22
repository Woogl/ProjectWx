# WxAI — AI 시스템

> 적/NPC의 Perception(시각·청각·피해) 감지, BehaviorTree 실행 노드, Blackboard 접근 규약, 정찰 경로를 제공하는 도메인 플러그인. AIController에 붙는 Perception 컴포넌트와 BT를 구성하는 커스텀 Task/Decorator/Service/Composite가 핵심이다.

## 책임
**담당**
- Perception 셋업과 TargetActor 확정·전투 인식(`State.InCombat`) 수명 관리 (`UWxAIPerceptionComponent`)
- Blackboard 키 이름·타입드 accessor 규약 (`WxBlackboardKeys`)
- BT 실행 노드 모음: 정찰/배회/복귀 이동, GAS 어빌리티 발동, 어트리뷰트 비율·리시 이탈 판정, 무작위 패턴 분기, 타겟 거리 갱신
- 정찰 경로 데이터(스플라인)와 순회 규칙 (`UWxPatrolComponent`)
- AI 청각용 소음 발생 AnimNotify (`UWxAnimNotify_ReportNoise`)

**경계 (비담당)**
- AIController·Pawn·BehaviorTree/Blackboard 에셋 자체는 게임 콘텐츠 측이 소유하고, 여기선 그것들이 조립해 쓰는 노드/컴포넌트만 제공한다.
- 어트리뷰트·어빌리티 정의와 전투 로직은 [[WxCombat]]. WxAI는 WxCombat에 의존하지 않으므로 어트리뷰트/AbilityTag는 디자이너가 BT 에디터에서 직접 지정한다.
- `State.InCombat` / `State.Dead` 등 태그는 소비만 한다(자체 Native Tag 없음).
- 복제된 `State.InCombat`을 읽는 네임플레이트·BGM은 소비자([[WxUI]]/[[WxSound]]) 책임이다.

## 의존성
- **주요 의존**: WxCore(공용 정의), 엔진 `AIModule`(BehaviorTree/Perception) · `GameplayAbilities`(GAS) · `GameplayTasks` · `NavigationSystem` · `GameplayTags`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`Plugins/WxAI/Source/WxAI/WxAI.Build.cs`의 Wx 의존은 `WxCore` 하나, `.uplugin`도 GameplayAbilities/WxCore만)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→Blackboard 타겟 동기화·전투 인식·회전 모드 발행의 런타임 허브. 대부분의 상태 흐름이 여기서 시작 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | BB 키 이름·타입드 accessor 규약(namespace). 모든 BT 노드가 이걸 경유해 키를 읽고 쓴다 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTComposite_RandomChoice` | 조건 필터 후 가중 랜덤으로 자식 1개만 실행. 적 공격 패턴 분기, `WxBTDecorator_RandomWeight`와 짝 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTDecorator_BeyondLeash` | 홈 이탈을 매 프레임 폴링·재평가하는 복귀 게이트. `WxBTTask_ReturnHome`과 한 쌍 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ActivateAbility` | BT에서 GAS 어빌리티를 태그로 발동하고 종료까지 대기 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTDecorator_AttributeRatio` | 어트리뷰트 비율(HP/MaxHP 등) 비교 조건 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h` |
| `UWxPatrolComponent` | 정찰 경로 스플라인. 상태 없는 경로 데이터(진행 커서는 BT Task가 폰별 소유) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- **새 BT 노드**: 엔진 베이스(`UBTTaskNode`/`UBTService`/`UBTDecorator`/`UBTCompositeNode`)를 상속하고 `WXAI_API`로 노출. 이동형 Task는 `UBTTask_MoveTo`를 상속해 이동/도착 판정을 엔진에 위임한다(`WxBTTask_Patrol`, `WxBTTask_ReturnHome` 참고).
- **Blackboard 접근**: 키를 문자열로 다루지 말고 반드시 `WxBlackboardKeys`의 타입드 accessor를 경유한다. 새 키는 `extern const FName` + accessor를 추가하고 Blackboard 에셋에 동명 키를 등록한다.
- **리시(leash) 브랜치 규약**: `WxBTDecorator_BeyondLeash`의 FlowAbortMode는 반드시 **Lower Priority**(Self/Both 금지 — 경계 왕복 유발), 복귀 브랜치는 전투 브랜치보다 상위 우선순위. 복귀 Task가 `SetTargetingSuppressed`로 재-어그로를 억제한다.
- **무작위 패턴**: `WxBTComposite_RandomChoice` 아래 각 자식에 `WxBTDecorator_RandomWeight`를 붙여 가중치를 운반(조건 아님, 항상 통과). 미부착 자식은 가중치 1.0, 0이면 추첨 제외.
- **권한 모델**: 소음 발생(`UWxAnimNotify_ReportNoise`)과 인식 태그 발행은 서버 전용. 인식은 MinimalReplication 태그로 클라에 복제된다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 감지→타겟→인식→회전 모드로 이어지는 상태 흐름의 출발점. 리시·복귀와의 역할 분담이 헤더 주석에 정리돼 있다.
2. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 모든 BT 노드가 공유하는 데이터 계약. 키별 accessor로 노드 간 무엇이 오가는지 한눈에 보인다.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` + `WxBTTask_ReturnHome.h` — 지각(억제)과 BT(게이팅)가 맞물리는 리시 메커니즘. 파일 횡단 협력의 대표 사례.
4. `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` — 가중 추첨·조건 필터·폴백 없음 시멘틱. 적 공격 패턴 분기의 핵심.

## 관련
- 상위: 게임 측 AIController/BehaviorTree 에셋이 이 노드·컴포넌트를 조립해 사용. 인식 태그 소비는 [[WxUI]]·[[WxSound]], 어트리뷰트·전투 정의는 [[WxCombat]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `b382b78` · 생성일 2026-07-22 · 소스 29파일 — `/readme-writer`로 갱신*
