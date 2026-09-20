# WxAI — AI 시스템

작업 단계: 구현

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../index.md) · [운영 절차](../maintenance.md)


> AI 폰의 판단 계층을 담당한다. 퍼셉션으로 잡은 자극을 Blackboard 타겟으로 정제하고, Behavior Tree 노드(Composite·Decorator·Service·Task)로 전투·정찰·소환수 행동을 조립한다.

## 책임
**담당**
- Blackboard 키 이름·타입의 단일 계약(`WxBlackboardKeys`)과 오용 진단
- 퍼셉션 감지 결과 → `TargetActor` 선정, `TargetDistance` 갱신 같은 "인지 → 블랙보드" 변환
- BT 노드 저작 팔레트: 무작위 분기, 리시(leash) 이탈 판정, 어트리뷰트 비율 조건, 정찰·배회·복귀 이동, 어빌리티 발동
- AI 락온(컨트롤러 포커스 + 폰 회전 모드)을 한 쌍으로 묶어 소유
- 소환수/분신 계열: Master의 커밋을 따라 같은 클래스·레벨을 부여하는 미러링, 지정 태그의 스킬에 반응하는 Follow 태스크, 이동 추종
- 폰에 붙는 AI 설정 보관(`UWxAIBehaviorComponent`: BT 에셋, 감각 수치, 정찰 경로, 피격 자극 보고)과 스플라인 정찰 경로 데이터(`UWxPatrolComponent`)

**경계 (비담당)**
- AIController 자체(퍼셉션 센스 구성, 팀 판정, `RunBehaviorTree`, 빙의 시 컨텍스트 키 세팅)는 `Source/WxGame/Controller/WxAIController.*`에 있다. 이 모듈은 그 컨트롤러가 호출하는 부품만 제공한다.
- 락온 "대상 보관"과 어트리뷰트/GameplayEffect 정의는 [WxCombat](WxCombat.md). WxAI는 이 모듈을 참조하지 않으므로, 어트리뷰트·감속 이펙트는 C++ 하드코딩 대신 BT 에디터에서 디자이너가 지정하는 방식으로 연결한다.
- 공용 GameplayTag 정의는 [WxCore](WxCore.md)(`WxGameplayTags`). 이 모듈은 태그를 선언하지 않고 소비만 한다.
- Behavior Tree/Blackboard 에셋 저작, 스포너 배치는 데이터 쪽 몫이다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxBlackboardKeys` | 모듈 전체가 공유하는 데이터 계약. 노드들은 서로 직접 대화하지 않고 전부 이 키를 거친다 — 여기부터 읽어야 노드 간 결합이 보인다 | [Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h) |
| `UWxAIBehaviorComponent` | 폰 쪽 AI 설정 보관소이자 컨트롤러와의 접점. 컨트롤러가 빙의 시 여기서 BT 에셋을 받아 실행하고, 감각 수치는 반대로 이 컴포넌트가 컨트롤러에 밀어 넣는다 | [Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h](../../../Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h) |
| `UWxBTService_UpdateTargetActor` | `TargetActor`의 유일한 발행자. 타겟 선정 규칙을 바꾸려면 이 노드만 본다 | [Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h) |
| `UWxBTService_LockOn` | 위가 발행한 타겟의 소비자. 컨트롤러 포커스와 폰 회전 모드를 단독 소유해 두 시스템의 상태 경합을 막는다 | [Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h) |
| `UWxBTTask_ActivateAbility` | BT → GAS 경계. 어빌리티 종료 통지를 BT 노드 결과로 번역한다 | [Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h) |
| `UWxBTComposite_RandomChoice` | 무작위 행동 선택의 축. `UWxBTDecorator_RandomWeight`와 반드시 한 쌍으로 읽는다(데코가 조건이 아니라 가중치 운반자다) | [Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h) |
| `UWxPatrolComponent` | 상태 없는 경로 데이터. 진행 커서는 `UWxBTTask_Patrol`이 폰별로 들고 있어 경로 공유·리스폰에 안전하다 | [Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h](../../../Plugins/WxAI/Source/WxAI/Public/WxPatrolComponent.h) |
| `UWxBTTask_MirrorAbility` | `MirrorTarget`(기본 Master) ASC의 커밋을 구독해 동일 클래스·레벨의 어빌리티를 별도 스펙으로 부여·발동한다 | [Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h) |

## 확장 포인트 / 규약
- **새 BT 노드 추가**: 엔진 베이스(`UBTTaskNode` / `UBTService` / `UBTDecorator` / `UBTCompositeNode`)를 상속한다. 고정 공용 키는 `WxBlackboardKeys` accessor를 사용한다. `MirrorTarget`·`ActorToObserve`처럼 저작자가 선택하는 키는 `FBlackboardKeySelector`와 선택된 이름으로 접근한다. 고정 키 추가 시 헤더·cpp와 Blackboard 에셋의 이름·타입을 맞춘다.
- **키 소유권**: `SelfActor`·`HomeLocation`·`Master` 초기화는 컨트롤러가 맡는다. `TargetActor`는 BT가 갱신하며 컨트롤러도 빙의·해제 시 비운다. `TargetDistance`·`PatrolTargetLocation`은 BT 노드가 쓴다. 타겟 부재 시 거리에는 `NoTargetDistance`가 들어간다.
- **WxCombat 비의존을 데이터로 메우는 지점**: `UWxBTDecorator_AttributeRatio`의 `Attribute`/`MaxAttribute`, `UWxBTTask_Patrol`·`UWxBTTask_Wander`의 `MoveSpeedEffect`는 C++에서 타입을 알 수 없어 BT 에디터에서 지정한다. 미지정 시 조용히 기본 동작(감속 없음)으로 떨어지므로 저작 누락에 주의한다.
- **정찰 배선**: 경로는 폰이 아니라 **스포너**에 붙인다. `UWxAIBehaviorComponent::InitializeComponent`가 빙의로 Owner가 덮이기 전에 스폰 주체에서 `UWxPatrolComponent`를 찾아 두는 순서에 의존하며, 이 순서는 `Private/Tests/WxPatrolPathSpawnTest.cpp`가 실제 스폰·빙의로 지킨다.
- **권한 모델**: 퍼셉션·BT·소음 보고(`UWxAnimNotify_ReportNoise`)는 모두 서버 권한 경로에서만 의미를 가진다. 이 모듈은 자체 리플리케이션 상태를 두지 않는다.

## 여기서부터 읽어라
1. [Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h) — 노드들이 무엇을 주고받는지, 누가 쓰고 누가 읽는지가 여기에 다 있다.
2. [Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp) — 폰 → 컨트롤러로 설정이 흘러가는 초기화 순서(빙의 타이밍 의존)를 확인한다.
3. [Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h) + [Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h](../../../Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h) — 타겟 "발행"과 "소비"를 분리한 이 모듈의 핵심 설계.
4. [Source/WxGame/Controller/WxAIController.cpp](../../../Source/WxGame/Controller/WxAIController.cpp) — 모듈 밖이지만, BT 실행과 컨텍스트 키 세팅이 실제로 일어나는 곳이라 함께 봐야 그림이 닫힌다.

## 검증 범위와 근거

[Build.cs](../../../Plugins/WxAI/Source/WxAI/WxAI.Build.cs)와 [descriptor](../../../Plugins/WxAI/WxAI.uplugin)의 WxCore·GAS 의존, Blackboard 계약, AIBehavior 초기화, 타겟 선정과 다음 소환수 경로를 정적으로 확인했다. 개별 BT 에셋의 실제 노드 조합과 순찰 스폰 테스트 실행은 미검증이다.

- [MirrorAbility.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp): `AbilityCommittedCallbacks` 구독, 정확 태그 기반 제외, SourceObject·실행 상태를 복사하지 않는 독립 스펙, 거절 시 RetryDuration 내 재시도, Master 교체·종료 시 자동 부여 스펙 정리.
- [FollowMasterAbility.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxBTTask_FollowMasterAbility.cpp): 지정 태그의 진행 중 스킬을 따라잡거나 다음 활성화를 기다리고, 반응 중에는 Master 구독을 해제한다. 자기 스킬 종료에 따라 태스크를 마감한다.
- [ObserveAbility.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp): 선택 액터의 활성 어빌리티를 검사하고, 액터 교체·활성화·종료에서 조건을 재평가한다.
- [MirrorMovement.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp): 서버에서 이동·점프·방향을 추종하고, 지정 스킬과 몽타주 종료에 따른 위치 보정을 처리한다.
- [UpdateTargetActor.cpp](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp): 유효한 기존 타겟은 유지하고, `Effect.IgnoreAggro` 또는 사망으로 자격을 잃으면 감지 기록을 제거한 뒤 새 후보를 고른다.

## 관련 모듈
- 상위: [Source/WxGame](../../../Source/WxGame) (AWxAIController, AWxEnemyCharacter가 이 모듈을 조립한다)
- 함께 보는 모듈: [WxCore](WxCore.md) (GameplayTag 정의), [WxCombat](WxCombat.md) (락온 대상 보관·어트리뷰트·GameplayEffect — 코드 의존 없이 BT 저작으로만 연결)

---
*이관 원문의 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 41파일 — 원문 출처 보존; 현재 확인 범위는 상단과 검증 절 참고*
