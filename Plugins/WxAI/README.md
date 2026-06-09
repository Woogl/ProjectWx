# WxAI — AI 시스템

> 적/NPC 폰의 행동을 구동하는 빌딩 블록을 제공한다. BehaviorTree 노드(Task/Decorator/Composite), AI Perception, Blackboard 키 규약, 팀 구분 타입을 한곳에 모은다.

## 책임
**담당**
- BehaviorTree 노드 제공 — GAS 어빌리티 발동, 배회 이동, 어트리뷰트 비율 조건, 무작위 분기 Composite.
- AI Perception 셋업(Sight/Hearing/Damage)과 그 결과를 Blackboard·폰 회전 모드·인식 태그로 동기화.
- Blackboard 키 이름 규약(`WxBlackboardKeys`)과 팀 구분 enum(`EWxTeam`) 정의.

**경계 (비담당)**
- 구체적인 적 BehaviorTree/Blackboard 에셋, AIController 클래스, 폰/캐릭터 정의는 게임 콘텐츠(`WxGame`)·데이터 에셋 쪽 소관이다.
- 어빌리티의 실제 구현은 [[WxCombat]] 소관이며, 이 모듈은 태그로만 발동을 요청한다.

## 의존성
- **주요 의존**: `WxCore`(공용 Gameplay Tag), `AIModule`, `GameplayAbilities`, `GameplayTasks`, `GameplayTags`, `NavigationSystem`
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | Sight/Hearing/Damage 감지 → TargetActor·회전 모드·인식 태그 동기화. 리시(leash) 기반 추적 유지/해제 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxBTTask_ActivateAbility` | AbilityTag로 ASC 어빌리티 발동, 종료까지 대기 후 결과 반환 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_Wander` | 폰 정면 기준 8방향 중 하나로 일정 시간 배회 이동 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h` |
| `UWxBTDecorator_AttributeRatio` | 어트리뷰트 비율(Attr/MaxAttr)을 기준값과 비교하는 조건 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h` |
| `UWxBTComposite_RandomChoice` | 자식 중 무작위 1개만 실행(논폴백), `bAvoidRepeat`로 직전 선택 회피 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_PrintString` | 화면/로그 출력 디버그 태스크, 항상 Succeeded | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_PrintString.h` |
| `WxBlackboardKeys` | Blackboard 키 이름 namespace(SelfActor/TargetActor/HomeLocation/TargetLastKnownLocation/Phase) | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `EWxTeam` | 캐릭터 팀 구분 enum(Player/Enemy/Neutral) | `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h` |

## Gameplay Tags
이 모듈은 C++ Native Tag를 **선언하지 않는다.** Perception이 인식 상태를 발행할 때 WxCore가 선언한 `WxGameplayTags::State_Recognized`를 폰 ASC에 MinimalReplication으로 부여/해제한다(네임플레이트·보스 체력바 표시에 소비). 선언처는 WxCore 참조.

## 확장 포인트 / 규약
- 새 BT 노드 추가 — Task는 `UBTTaskNode`, Decorator는 `UBTDecorator`, Composite는 적절한 베이스(`UBTComposite_*`)를 상속하고 접두사 `WxBT...`를 따른다. Composite에서 자체 노드 메모리가 필요하면 `FWxBTRandomChoiceMemory`처럼 `FBTCompositeMemory` 뒤에 상태를 배치한다(엔진 메모리 레이아웃 보존).
- 데이터 주도 — 적 행동은 이 모듈의 노드를 조합한 BehaviorTree·Blackboard 에셋(게임 콘텐츠 쪽)이 구동한다. Blackboard 에셋에는 `WxBlackboardKeys`와 같은 이름의 키가 등록돼 있어야 한다. 어빌리티 발동·어트리뷰트 비교는 디자이너가 BT 에디터에서 태그/`FGameplayAttribute`를 직접 지정한다(WxAI는 WxCombat에 의존하지 않으므로 어트리뷰트 식별자를 하드코딩하지 않는다).
- 리플리케이션/권한 — `UWxAIPerceptionComponent`의 인식·추적 판정은 서버 권한에서 수행되며, `State.Recognized`만 MinimalReplication으로 클라에 복제된다(최대 4인 멀티). TargetActor/회전 모드 발행은 서버 측에서 BB·MovementComponent에 직접 반영된다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 확정/리시 해제/회전 모드/인식 태그가 한 클래스에 모여 있어 AI 흐름의 중심. 주석이 상태 수명을 상세히 설명한다.
2. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 노드와 Perception이 공유하는 키 규약. 어떤 정보가 Blackboard로 오가는지 한눈에 본다.
3. `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` — 보스/적 공격 패턴 분기의 핵심 시멘틱(논폴백 무작위 선택)과 BT 노드 메모리 패턴 예시.

## 관련
- 상위: 적/보스 BehaviorTree·AIController·폰 정의(게임 콘텐츠/`WxGame`)가 이 모듈의 노드와 컴포넌트를 소비한다.
- [[WxCombat]] — 어빌리티(GAS)·어트리뷰트의 실제 구현처. WxAI는 태그/어트리뷰트 핸들로만 참조하며 직접 의존하지 않는다.
- [[WxCore]] — `State.Recognized` 등 공용 Gameplay Tag 선언처.

---
*문서 기준 커밋 `d60410d8` · 생성일 2026-06-09 · 소스 17파일 — `/readme-writer`로 갱신*
