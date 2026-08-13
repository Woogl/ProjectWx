# WxAI — AI 시스템

> 적 AI의 지각(Perception)과 BehaviorTree 노드 라이브러리. 감지→타겟 확정→전투/복귀/정찰의 흐름을 구동하는 커스텀 Task/Decorator/Service/Composite와 Blackboard 규약을 제공한다.

## 책임
**담당**
- Perception 셋업과 타겟 수명 관리: Sight/Hearing/Damage 감지 → Blackboard `TargetActor` 동기화, 인식(`State.InCombat`) 발행, 타겟 사망/소실 정리, 전투 시 회전 모드(strafe) 전환 (`UWxAIPerceptionComponent`)
- 리시(leash) 이탈 판정과 배치 지점 복귀 (`UWxBTDecorator_BeyondLeash` + `UWxBTTask_ReturnHome`)
- 데이터 주도 전투 패턴 분기: 유효 후보 중 가중 무작위 선택 (`UWxBTComposite_RandomChoice` + `UWxBTDecorator_RandomWeight` + `UWxBTService_TargetDistance` + `UWxBTDecorator_AttributeRatio`)
- 정찰/배회 이동: 스플라인 경로 데이터와 순회 규칙(`UWxPatrolComponent`), 경로 추종(`UWxBTTask_Patrol`), 방향 배회(`UWxBTTask_Wander`)
- GAS 어빌리티 발동 Task (`UWxBTTask_ActivateAbility`), 소음 발생 AnimNotify (`UWxAnimNotify_ReportNoise`)
- Blackboard 키 이름·타입드 accessor 규약 (`WxBlackboardKeys`), 팀 구분 열거형 (`EWxTeam`)

**경계 (비담당)**
- AIController/캐릭터 실체, 폰 스폰·생명주기는 [[WxGame]]에 위임 (여기의 컴포넌트를 부착·소유하는 쪽)
- 어트리뷰트/GameplayEffect 정의는 [[WxCombat]]에 위임 — WxAI는 의존하지 않고, 디자이너가 BT 에디터에서 `FGameplayAttribute`/`TSubclassOf<UGameplayEffect>`를 직접 지정한다

## 의존성
- **주요 의존**: `WxCore`, 엔진 `AIModule`, `GameplayAbilities`(GAS), `GameplayTasks`, `NavigationSystem`
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`WxAI.Build.cs`·`WxAI.uplugin` 모두 Wx 의존은 `WxCore`뿐. WxCombat 미의존은 의도적이며, 어트리뷰트/GE는 BT 에디터에서 주입)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | 감지→`TargetActor`/인식 동기화, 회전 모드·억제 발행 | `Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `WxBlackboardKeys` | BB 키 이름 + 타입드 accessor 규약 (namespace) | `Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxBTComposite_RandomChoice` | 유효 후보 중 가중 무작위 1개 실행 (패턴 분기) | `Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | 태그로 GAS 어빌리티 발동, 종료까지 대기 | `Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTDecorator_BeyondLeash` | 배치 지점 이탈 판정, 실시간 abort로 복귀 게이팅 | `Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` |
| `UWxBTTask_ReturnHome` | 추격 포기·홈 복귀, 진입 시 타겟 억제 지시 | `Source/WxAI/Public/WxBTTask_ReturnHome.h` |
| `UWxPatrolComponent` | 스플라인 정찰 경로 데이터 + 순회 규칙(무상태) | `Source/WxAI/Public/WxPatrolComponent.h` |
| `EWxTeam` | 팀 구분(Player/Enemy/Neutral) | `Source/WxAI/Public/WxTeamTypes.h` |

## 확장 포인트 / 규약
- **새 BT 노드**: 엔진 베이스(`UBTTaskNode`/`UBTDecorator`/`UBTService`/`UBTCompositeNode`)를 상속하고 `WXAI_API`로 노출한다. 노드별 상태는 Blackboard가 아니라 노드 인스턴스에 두는 패턴을 따른다(`UWxBTTask_Patrol`은 `bCreateNodeInstance`로 폰별 커서 보관 → 경로 공유·리스폰 안전).
- **Blackboard 규약**: 키는 이름 직접 참조 대신 `WxBlackboardKeys`의 accessor로 읽고 쓴다(타입 오용·stale 값 방지). 키 SET/CLEAR는 AIController 또는 `UWxAIPerceptionComponent`가, 참조는 Task/Service/관찰자가 담당. Blackboard 에셋에 동일 이름 키가 등록돼 있어야 한다.
- **WxCombat 비의존 데이터 주입**: 감속 GE(`MoveSpeedEffect`)·어트리뷰트 쌍(`AttributeRatio`)은 코드가 아니라 BT 에디터에서 디자이너가 지정한다. 새 노드도 이 규약을 지켜 WxCombat 링크를 만들지 말 것.
- **가중 추첨 관례**: `RandomChoice`의 자식은 `RandomWeight` Decorator로 가중치를 실어 나른다(조건 아님, 항상 true). 조건 Decorator(`AttributeRatio` 등)는 후보 필터로, `RandomWeight`는 확률로 동작.
- **리시 협업**: 타겟 유지·정리는 Perception이, 이탈 판정·복귀는 BT가 담당하는 분업. `BeyondLeash`는 반드시 FlowAbortMode=Lower Priority로 설정(Self/Both 금지 — 경계 왕복 유발).

## 여기서부터 읽어라
1. `Source/WxAI/Public/WxAIPerceptionComponent.h` — 감지부터 타겟 확정·인식·회전·복귀 억제까지 전 흐름의 허브. 헤더 주석이 수명·협업 관계를 상세히 설명한다.
2. `Source/WxAI/Public/WxBlackboardKeys.h` — 노드들이 공유하는 데이터 계약. 어떤 키를 누가 쓰고 읽는지 파악하면 나머지 노드가 이어진다.
3. `Source/WxAI/Public/WxBTComposite_RandomChoice.h` — 전투 패턴 분기의 핵심. RandomWeight/AttributeRatio/TargetDistance가 이 컴포지트를 중심으로 맞물린다.

## 관련
- 상위: [[WxGame]] — `WxEnemyController`가 `UWxAIPerceptionComponent`를 부착하고, `WxEnemyCharacter`/`WxCharacterBase`가 이 노드들이 구동하는 폰. 스폰·정찰 경로 연결은 `AWxSpawner`.

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 29파일 — `/readme-writer`로 갱신*
