# WxAI — AI 시스템

> AI 폰의 판단 계층을 담당한다. 퍼셉션으로 잡은 자극을 Blackboard 상태로 바꾸고, 그 상태를 읽는 Behavior Tree 노드(Composite/Decorator/Service/Task)를 제공해 전투·정찰·배회·리시 복귀·분신 미러링을 굴린다.

## 책임

**담당**
- Blackboard 키 계약(`WxBlackboardKeys`) — 이 모듈의 노드들이 서로 대화하는 유일한 통로다. 키 이름·값 타입·"타겟 없음" 표현(`NoTargetDistance`)을 한 곳에 묶는다.
- 퍼셉션 결과의 해석 — 감지된 액터 중 누구를 `TargetActor`로 삼을지(사망·어그로 비허용 필터), 거리 갱신, 락온 적용/해제.
- BT 노드 저작 팔레트 — 가중 무작위 분기, 어빌리티 발동, 정찰/배회/복귀 이동, Attribute 비율 게이트, 리시 반경 게이트.
- 정찰 경로 데이터(`UWxPatrolComponent`)와 그 순회 규칙(PingPong/Loop/Once). 진행 커서는 갖지 않는다.
- 소음 자극 발생 지점(`UWxAnimNotify_ReportNoise`)과 피격 자극 보고(`UWxAIBehaviorComponent`).
- Master(소환자) 행동 미러링 — Master ASC 구독, 어빌리티 태그 분기, 미러 어빌리티 발동, 옆자리 추종 이동.

**경계 (비담당)**
- AIController·퍼셉션 센스 구성·BT 실행 자체는 `AWxAIController`([[WxGame]], `Source/WxGame/Controller/WxAIController.h`)가 소유한다. 이 모듈은 노드와 데이터만 제공하고 컨트롤러를 정의하지 않는다.
- 어빌리티/Attribute/이동속도 GE의 정의는 [[WxCombat]]. WxAI는 `FGameplayAttribute`·`TSubclassOf<UGameplayEffect>`를 **저작 값으로 받아** 다룰 뿐 타입을 알지 못한다(`UWxBTDecorator_AttributeRatio`, `UWxBTTask_Patrol`/`_Wander`의 `MoveSpeedEffect`).
- 락온 **대상 보관**은 [[WxCombat]]의 `UWxLockOnComponent`이며, BT가 고른 타겟을 그쪽에 옮기는 일은 `AWxAIController`가 한다. 이 모듈의 `UWxBTService_LockOn`은 "어떻게 바라볼지"(컨트롤러 포커스 + 폰 strafe 회전 모드)만 정한다.
- 소환자-소환물 관계(`UWxMinionSubsystem`, `IWxMinion`)와 공용 태그(`WxGameplayTags`)는 [[WxCore]].
- 스폰·배치·경로 소유 액터는 [[WxWorld]]/레벨 저작 쪽. `UWxPatrolComponent`는 폰의 **부착 부모**에서 경로를 찾을 뿐이다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 모듈 전체의 상태 계약. 어떤 노드가 어떤 키를 쓰는지 여기서부터 역추적한다 | `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` |
| `UWxAIBehaviorComponent` | 폰 쪽 저작 진입점. BT 에셋과 감각 수치를 들고 있다가 빙의 시 컨트롤러로 밀어 넣는다 | `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` |
| `UWxBTService_UpdateTargetActor` | 퍼셉션 → Blackboard 변환 지점. 전투 파이프라인의 시작 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h` |
| `UWxBTService_LockOn` | `TargetActor`의 소비 지점. 컨트롤러 포커스와 폰 회전 모드를 한 쌍으로 단독 소유한다 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h` |
| `UWxBTTask_ActivateAbility` | BT ↔ GAS 경계. 어빌리티 수명을 노드 결과로 번역한다 | `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` |
| `UWxBTComposite_RandomChoice` | 전투 패턴 분기의 뼈대. `UWxBTDecorator_RandomWeight`와 한 쌍으로만 의미가 있다 | `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` |
| `UWxPatrolComponent` | 무상태 경로 제공자. 커서는 `UWxBTTask_Patrol`이 폰별로 소유한다 | `Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h` |
| `UWxBTService_ObserveMasterAbility` | 분신(미러) 계열 진입점. Master ASC 구독을 최상위 분기 수명에 건다 | `Plugins/WxAI/Source/WxAI/Public/WxBTService_ObserveMasterAbility.h` |

## 확장 포인트 / 규약
- **새 BT 노드 추가**: 엔진 베이스(`UBTService`/`UBTTaskNode`/`UBTDecorator`/`UBTCompositeNode`)를 그대로 상속하고 `Public/`에 `WxBT<종류>_<이름>.h`로 둔다. 상태를 갖는 노드는 두 방식 중 하나를 고른다 — 폰별 필드를 그냥 쓰려면 생성자에서 `bCreateNodeInstance = true`(`UWxBTTask_ActivateAbility`, `UWxBTTask_Patrol`), 인스턴스 메모리를 쓰려면 `GetInstanceMemorySize`/`InitializeMemory` 쌍(`UWxBTService_LockOn`, `UWxBTDecorator_BeyondLeash`). Composite 메모리는 반드시 `FBTCompositeMemory`를 상속해 앞쪽을 덮지 않게 한다.
- **새 Blackboard 키 추가**: `WxBlackboardKeys.h/.cpp`에 이름과 타입 맞춤 accessor를 같이 넣고, Blackboard 에셋에 동명 키를 등록한다. 노드에서 `GetValueAsX`를 직접 부르지 않는다(타입 오용·무경고 기본값 반환 방지).
- **모듈 의존 규칙의 실제 모양**: WxAI는 [[WxCombat]]을 참조하지 않으므로, 전투 쪽 애셋이 필요한 노드는 전부 **에디터 저작 값으로 주입**받는다 — `UWxBTDecorator_AttributeRatio.Attribute/MaxAttribute`(예: `WxCombatAttributeSet::HP`/`MaxHP`), `UWxBTTask_Patrol`/`UWxBTTask_Wander`의 `MoveSpeedEffect`(`WxEffect_MoveSpeedScale`, SetByCaller로 배율 전달). 저작 누락 시 기능이 조용히 꺼진다(감속 없음 등).
- **데이터 주도 설정**: 행동은 `UWxAIBehaviorComponent.BehaviorTreeAsset`의 BT/Blackboard 에셋이 구동한다. 어빌리티는 클래스가 아니라 `AbilityTag`(Ability.*)로 지목하므로, 폰 ASC에 해당 태그를 가진 어빌리티가 미리 부여돼 있어야 한다. 분신 매핑은 `FWxMirrorAbilityMapping` 배열(원본 어빌리티/몽타주 → 미러 어빌리티)로 노드에 직접 저작한다.
- **권한 모델**: 퍼셉션·BT·GAS 발동이 모두 서버 권한 경로다. `UWxAnimNotify_ReportNoise`는 서버에서만 자극을 발생시킨다.

## 여기서부터 읽어라
1. `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` — 노드 간 유일한 통신 통로라, 키 목록과 "누가 SET하고 누가 읽는가"를 먼저 잡으면 나머지 노드가 전부 이 위에 얹힌 것으로 읽힌다.
2. `Source/WxGame/Controller/WxAIController.cpp` — 이 모듈 밖이지만, BT를 실제로 돌리고 `SelfActor`/`HomeLocation`/`Master`를 심는 곳이다. 여기를 보지 않으면 Blackboard 초기 상태의 출처를 알 수 없다.
3. `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp` → `Public/WxBTService_LockOn.h` — 감지에서 조준까지의 한 줄기 흐름. 발행(TargetActor)과 소비(포커스·회전)가 왜 두 노드로 갈라져 있는지가 이 모듈의 설계 기조다.
4. `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` — Selector와 시멘틱이 다르고 Decorator 평가·가중치 필터를 직접 하므로, 전투 BT를 읽기 전에 규칙을 알아야 한다.
5. `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h` — BT 결과와 GAS 수명(동기 종료·abort 거부)을 잇는 지점이라 전투 브랜치 디버깅이 대개 여기로 수렴한다.

## 관련
- 상위: [[WxGame]] — `AWxAIController`가 BT를 실행하고, `AWxEnemyCharacter`가 `UWxAIBehaviorComponent`/정찰 경로를 물고 있다.
- [[WxCore]] — 유일하게 참조하는 Wx 플러그인. `WxGameplayTags`(Event.Hit, Ability.Death, SetByCaller.MoveSpeedScale)와 `IWxMinion`(어그로 제외 판정)을 쓴다.
- [[WxCombat]] — 코드 의존은 없고 저작 값으로만 만난다(Attribute, GameplayEffect, 락온 대상 보관).

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 40파일 — `/readme-writer`로 갱신*
