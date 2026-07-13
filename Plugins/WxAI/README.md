# WxAI — AI 시스템

> 적/보스 폰의 지각(Perception), BehaviorTree 노드(Task/Service/Decorator/Composite), Blackboard 키 규약, 정찰 경로를 제공하는 도메인 플러그인. 데이터 주도로 디자이너가 BT 에디터에서 조합한다.

## 책임
**담당**
- 지각·추적: 시각/청각/피해 감지, TargetActor 확정, 회전 모드(strafe) 발행, 사망·복귀 시 인식 해제. 리시 복귀 진입 시 타겟 억제(disengage) (`UWxAIPerceptionComponent`)
- 리시(leash) 판정·복귀: 홈 이탈 판정 데코레이터 + 복귀 태스크로 BT가 소유(데이터 주도, 반경은 디자이너 지정)
- Blackboard 키 이름 + 타입드 accessor 규약 (`WxBlackboardKeys`)
- 커스텀 BT 노드: 어빌리티 발동, 정찰/배회 이동, 리시 복귀, 거리 서비스, 어트리뷰트 비율·랜덤 가중·리시 이탈 데코레이터, 랜덤 선택 컴포지트
- 정찰 경로 데이터(스플라인)와 순회 규칙 (`UWxPatrolComponent`), 팀 구분 enum (`EWxTeam`)

**경계 (비담당)**
- 어빌리티·어트리뷰트의 실제 구현 — [[WxCombat]] (WxAI는 태그/핸들로만 발동·비교를 요청하며 직접 의존하지 않음)
- 인식 태그(`State.InCombat`) 선언 및 공용 Gameplay Tag — [[WxCore]]
- 적 스폰·배치, 구체 BT/Blackboard 에셋·AIController·폰 정의 — 게임 콘텐츠/[[WxWorld]] (정찰 컴포넌트는 스폰된 폰이 조회만 함)

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존, `WxGameplayTags.h`), `AIModule`, `GameplayAbilities`, `GameplayTasks`, `NavigationSystem`, `GameplayTags`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxBTDecorator_AttributeRatio`가 어트리뷰트를 디자이너 지정에 맡겨 WxCombat 의존을 의도적으로 회피)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAIPerceptionComponent` | AIController에 부착. Sight/Hearing/Damage → TargetActor/LastKnown 동기화, strafe 회전 모드 발행, 사망·복귀 시 인식 해제. 리시 복귀는 `SetTargetingSuppressed`로 타겟/재감지 억제 | `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` |
| `UWxBTDecorator_BeyondLeash` / `UWxBTTask_ReturnHome` | 폰이 HomeLocation에서 LeashRadius 이상 이탈했는지 판정하는 조건 데코 / 이탈 시 타겟 억제 후 Home으로 MoveTo 복귀하는 태스크. 리시 검출·복귀를 퍼셉션 폴에서 BT로 이관 | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` · `WxBTTask_ReturnHome.h` |
| `WxBlackboardKeys` | BB 키 이름 + 타입드 accessor namespace. Perception/AIController가 SET, BT 노드가 참조하는 데이터 계약 허브 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxPatrolComponent` | 스플라인 기반 정찰 경로(무상태). MoveMode(PingPong/Loop/Once) 규칙, `FindPatrolComponent`로 조회. 커서는 BT 태스크가 폰별 소유 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTComposite_RandomChoice` | 자식 1개를 가중 랜덤 선택(폴백 없음). 공격 패턴 분기용. `bAvoidRepeat`로 직전 선택 회피 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_ActivateAbility` | AbilityTag로 ASC 어빌리티 발동, 종료까지 대기 후 결과 반환 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTTask_Patrol` / `UWxBTTask_Wander` | `MoveTo` 상속 정찰(도착 시 커서 진행) / 폰 정면 기준 8방향 랜덤 배회. 폰별 노드 인스턴스 상태 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxBTService_TargetDistance` | Self↔Target 거리를 `TargetDistance`(Float 키)에 주기 기록 → 엔진 기본 arithmetic 데코레이터가 근/원거리 분기에 소비 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_TargetDistance.h` |
| `UWxBTDecorator_AttributeRatio` / `_RandomWeight` | 어트리뷰트 비율(Attr/MaxAttr) 비교 조건 / RandomChoice용 가중치 운반 데코레이터(조건 아님) | `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h` |

## 확장 포인트 / 규약
- **새 BT 노드**: Task는 `UBTTaskNode`(또는 `UBTTask_MoveTo` 등 엔진 파생), Service는 `UBTService`, Decorator는 `UBTDecorator`, Composite는 적절한 `UBTComposite_*`를 상속하고 접두사 `WxBT...`를 따른다. `GetStaticDescription` 오버라이드로 에디터 표시를 채운다. Blackboard 접근은 반드시 `WxBlackboardKeys` accessor 경유(타입 오용·키 이름 산개 방지).
- **폰별 상태**가 필요한 Task는 노드 인스턴스(`bCreateNodeInstance`, 멤버 변수)에 보관 — 같은 경로/트리를 여러 폰이 공유하고 리스폰해도 안전(`Patrol`/`Wander` 참조). Composite에서 자체 노드 메모리가 필요하면 `FWxBTRandomChoiceMemory`처럼 `FBTCompositeMemory` 뒤에 상태를 배치(엔진 메모리 레이아웃 보존).
- **RandomChoice 가중치**: 자식에 `UWxBTDecorator_RandomWeight`를 붙여 Weight 지정(무부착=1.0, 0=추첨 제외). 조건 평가가 아닌 데이터 운반 데코레이터.
- **거리 분기**: `TargetDistance` 서비스 + 엔진 기본 Blackboard 산술 데코레이터로 근접/원거리 분기(커스텀 데코레이터 불필요).
- **새 Blackboard 키**: `WxBlackboardKeys`에 `extern const FName`과 타입드 accessor를 함께 선언/정의. Object 키는 nullptr Set이 Clear와 동치이나, Vector/Float 키(`TargetLastKnownLocation`, `TargetDistance`)는 "값 없음"을 Set으로 표현할 수 없어 별도 Clear accessor를 둔다.
- **어트리뷰트 비교**: WxCombat 비의존 유지를 위해 Attribute/MaxAttribute를 BT 에디터에서 `FGameplayAttribute`로 직접 지정(예: `WxCombatAttributeSet::HP`/`MaxHP`).
- **리플리케이션**: Perception의 인식·추적 판정은 서버 권한에서 수행되고, `State.InCombat`만 MinimalReplication으로 클라에 복제(네임플레이트 소비). TargetActor/회전 모드는 서버에서 BB·MovementComponent에 직접 반영.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — Perception·AIController·BT 노드가 데이터를 주고받는 중앙 계약. 시스템 전체 데이터 흐름의 허브.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` — 타겟 확정/회전 모드/인식 태그/억제(disengage)가 한 클래스에 모여 AI 상태 전환의 근원. 리시 판정은 BT로 이관됐고, 주석이 상태 수명을 상세히 설명한다.
3. `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp` — 노드 메모리 레이아웃과 `GetNextChildHandler` 가중 룰렛. 커스텀 컴포지트 작성 패턴 참고.

## 관련
- 상위: [[WxCore]]
- 소비: [[WxCombat]](어빌리티/어트리뷰트), [[WxWorld]](스폰·정찰 배치), [[WxUI]](InCombat 네임플레이트)
---
*문서 기준 커밋 `d0c804a` · 생성일 2026-07-12 · 소스 25파일 — `/readme-writer`로 갱신*
