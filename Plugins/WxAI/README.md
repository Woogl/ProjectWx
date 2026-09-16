# WxAI — AI 시스템

> 적 폰의 행동을 굴리는 재료를 모은 런타임 플러그인. Behavior Tree 노드(Task·Service·Decorator·Composite), 퍼셉션·피격 자극 배선, Blackboard 키 규약, 정찰 경로 데이터를 제공한다.

## 책임
**담당**
- BT 노드 저작 재료: 어빌리티 발동/미러, 정찰·복귀·배회 이동, 타겟 갱신·거리 갱신·락온 서비스, 리시·속성비·가중치 데코레이터, 무작위 선택 컴포짓
- 캐릭터가 AI로 구동될 때 필요한 것(BT 애셋·감각 수치·피격 자극 보고)을 폰에 얹는 `UWxAIBehaviorComponent`
- Blackboard 키 이름·값 타입을 한곳에 묶은 accessor 규약(`WxBlackboardKeys`)과 키 소유 분담
- 정찰 경로 데이터(`UWxPatrolComponent`)와 소음 발생 애님 노티파이

**경계 (비담당)**
- AIController·Blackboard SET 진입점·락온 대상 선정은 안 한다 — 컨트롤러는 `Source/WxGame/Controller/WxAIController.h`(AWxAIController)가, 겨눌 대상은 [[WxCombat]]의 `UWxLockOnComponent`가 소유한다. 이 모듈은 그 대상을 어떻게 바라볼지만 정한다.
- 전투 규칙(GameplayEffect·AttributeSet)을 참조하지 않는다 — WxCombat에 의존하지 않는 것이 설계. 감속 이펙트·속성 키는 디자이너가 BT 에디터에서 직접 지정한다. [[WxCombat]]
- BT 애셋·Blackboard 애셋 자체는 데이터로, 이 모듈은 그것을 소비하는 C++ 노드만 제공한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 키 이름+타입 accessor 네임스페이스. 키 SET/CLEAR 소유 분담(컨트롤러 vs 노드)을 헤더 주석이 지도로 정리 — 데이터 흐름 시작점 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIBehaviorComponent` | 캐릭터에 붙어 BT·감각·피격 자극을 캐릭터에 따라다니게 함. 컨트롤러 빙의 시점에 감각 수치를 퍼셉션에 밀어 넣음 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `UWxBTService_UpdateTargetActor` | 퍼셉션 감지 → Blackboard TargetActor 발행. 적 감지 파이프라인의 입구 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | TargetActor를 컨트롤러 포커스+폰 strafe 회전 모드로 반영. AI판 락온(플레이어 락온과 별개) | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | GAS 어빌리티를 태그로 발동하고 종료까지 latent로 대기 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 조건 통과 자식 중 무작위 1개 실행. `RandomWeight` 데코로 가중, Selector 폴백 없음 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxBTTask_Patrol` | `UBTTask_MoveTo` 상속, 도착 시 `UWxPatrolComponent`에 정찰 커서 진행 위임 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_Patrol.h` |
| `UWxPatrolComponent` | 스플라인 포인트를 정찰 지점으로 제공하는 무상태 경로 데이터(커서는 BT 태스크가 폰별 소유) | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |

## 확장 포인트 / 규약
- 새 BT 노드는 엔진 베이스(`UBTTaskNode`·`UBTService`·`UBTDecorator`·`UBTCompositeNode`)를 상속하고 `WXAI_API`로 노출한다. 인스턴스별 상태가 필요하면 `GetInstanceMemorySize`/`InitializeMemory`로 노드 메모리에 실거나 `bCreateNodeInstance`를 쓴다(`WxBTTask_Patrol` 참고).
- Blackboard 값은 `GetValueAs`/`SetValueAs` 직접 호출 대신 `WxBlackboardKeys`의 키별 accessor를 통한다(타입 오용·조용한 기본값 반환을 경고 로그로 드러냄). 새 키는 accessor를 추가하고 Blackboard 애셋에 동명 키를 등록해야 한다.
- 데이터 주도: WxCombat에 의존하지 않으므로 전투 연동은 BT 에디터에서 디자이너가 지정한다 — 정찰 감속은 `MoveSpeedEffect`(`WxEffect_MoveSpeedScale` 류 GameplayEffect), 속성 게이팅은 `AttributeRatio` 데코의 `Attribute`/`MaxAttribute`(예: `WxCombatAttributeSet::HP`), 어빌리티는 태그(`Ability.*`)로 지목한다.
- 권한: 소음 보고(`UWxAnimNotify_ReportNoise` → `UAISense_Hearing::ReportNoiseEvent`)와 퍼셉션은 서버 전용. 피격 자극은 폰이 컨트롤러에 빙의돼 있을 때(=AI 조종 중)만 리스너가 있어 유효하다.

## 미니언의 Master 반응

- `BT_Minion` 최상위의 `UWxBTService_ObserveMasterAbility`가 서버에서 Blackboard `Master` ASC의 기존 발동 델리게이트를 구독한다. 폴링이나 별도 GameplayEvent는 사용하지 않는다.
- 소환 도중 이미 시작된 스킬은 ASC에 처음 연결할 때 한 번 따라잡는다. 트리 재탐색으로 같은 실행을 다시 요청하지 않는다.
- `BT_Minion`은 기존 `BB_Shared`를 그대로 사용한다. 추가 BB 키 없이 감지 Service 인스턴스의 `PendingAbility` 태그 하나를 임시 보관한다. 감지 번호나 별도 실행 상태는 두지 않는다.
- `UWxBTDecorator_MasterAbility`가 Master 태그를 정확히 비교하고, 분기 진입 시 요청을 한 번 소비한다. 별도 소비 Task 없이 기존 `UWxBTTask_ActivateAbility`를 바로 배치한다. 대응 스킬은 발동 Task의 `AbilityTag`에서 정한다.
- 최상위 Selector 아래에는 Skill 1 단일 Task, Skill 2·강공격의 공격→퇴장 Sequence, 리시 이탈 퇴장, 대기의 다섯 분기만 둔다. 대응하지 않는 Master 태그는 실행하지 않으며 다음 감지로 교체된다.
- 새 발동을 감지하면 태그를 보관하고 부모 Selector에서 반응을 다시 선택한다. 실행 중인 반응은 기존 Task의 Abort로 취소하며 같은 스킬도 재시작한다. BT 좌우 우선순위와 무관하게 전환하고, 재탐색 전 여러 발동은 최신 태그 하나만 반영한다. 취소 불가·쿨다운 등 GAS 제한은 유지하며 발동 실패를 자동 재시도하지 않는다. Master 종료만으로 미니언 스킬을 취소하지는 않는다.
- 현재 Skill 1은 실행 후 대기하고, Skill 2·강공격은 정상 종료 후 BT에서 `Ability.Death`를 실행해 퇴장한다. 기존 리시 이탈 퇴장 분기는 유지한다. 발동은 수정하지 않은 기존 `UWxBTTask_ActivateAbility`가 담당한다.
- `Log LogWxAI Verbose`로 Master 발동 감지를 확인한다. Service 런타임 표시에는 감지한 태그가 나오며, 실행 분기는 BT 디버거에서 확인한다. 발동 Task는 Master 상태를 참조하지 않는다.

## 도플갱어의 Master 복제

- `BT_Doppelganger`는 `BB_Shared.Master`와 기존 `AWxAIController`를 사용한다. 루트 Sequence에 `MirrorMovement` Service, 그 아래에 지속 실행되는 `MirrorAbility` Task를 배치한다. `BT_Minion`의 반응 분기는 변경하지 않는다.
- `MirrorMovement`는 Master의 우측 100cm를 목표로 CMC 이동 입력을 넣는다. 도달 반경은 15cm이며 일반 지상 추종이 연속 1초 이상 걸리면 충돌 검사 후 텔레포트한다. Master 또는 분신의 어빌리티가 활성인 동안에는 텔레포트를 막고 접근 타이머를 초기화한다. GAS 종료 통지 후 양쪽의 활성 어빌리티가 모두 없어진 첫 BT 틱에는 1초를 기다리지 않고 현재 Master 우측 100cm로 텔레포트한다. 목적지 충돌로 실패하면 복귀 요청을 유지한다. 점프·추가 점프·앉기·회전도 따르고 공중에서는 일반 추종 타이머를 초기화한다. 상대 위치를 고정하는 부착은 사용하지 않는다.
- `MirrorAbility.AbilityMappings`는 원본 클래스와 실제 재생 몽타주를 분신용 클래스에 대응시킨다. GAS 커밋 이후 BT 틱에서 읽으므로 같은 태그의 콤보·회피 변형을 구분하며, 여러 활성 행동을 각 스펙별로 추적한다. 패시브는 `ReplayOnSuccessfulEnd`, 대상이 필요한 처형은 `SourceEventTag`를 사용한다.
- 분신 전용 에셋에서 비용·쿨다운·입력 자격 조건을 비우고, 콤보는 단일 단계로 구성한다. 소환 노티파이가 있는 몽타주는 분신용 복사본에서 해당 노티파이를 제거한다. 처형의 대상 정보는 원본 이벤트를 전달하되 분신용 피해자 몽타주를 비워 대상 연출을 중복 재생하지 않는다. 기존 `WxAbility` 코드는 수정하지 않는다.
- HGTest 약공격 4타의 `WxAnimNotify_SpawnMinion.BlockingMinionClass`는 BP_Doppelganger다. Master의 활성 도플갱어가 있으면 일반 미니언 소환을 건너뛰며, 도플갱어가 없으면 기존 소환·교체 규칙을 따른다. 궁극기 소환에는 이 제한을 설정하지 않는다.
- Task 종료·중단 시 Master 구독과 해당 Task가 부여한 어빌리티를 정리한다. Service도 이동 설정과 틱 의존성을 되돌린다. 새 어빌리티/몽타주를 HGTest에 추가하면 대응 에셋과 매핑도 추가해야 한다. 이벤트를 놓친 뒤 이미 진행 중인 처형에 붙는 경우에는 대상 문맥을 추측하지 않고 다음 발동을 기다린다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 키 소유 분담 주석이 이 모듈 전체의 데이터 흐름 지도다.
2. `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` — 캐릭터가 AI로 구동되기 시작하는 지점(BT·감각·자극 배선).
3. `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` + `WxBTService_LockOn.h` — 감지 → TargetActor → 조준으로 이어지는 파이프라인.

## 관련
- 상위: `Source/WxGame/Controller/WxAIController.h`(AWxAIController)가 이 모듈의 BT 노드·Blackboard 키를 구동하고 SelfActor·HomeLocation·Master를 SET 한다. 락온 대상 선정은 [[WxCombat]] `UWxLockOnComponent`가 담당한다.

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 34파일 — `/readme-writer`로 갱신*
