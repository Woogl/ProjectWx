---
title: "AI 트리 정지·잠금을 AWxAIController 단독으로: 그로기는 Reaction 잠금, 돌진은 관여 안 함"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, ai, combat, architecture]
summary: "돌진 modifier와 그로기 어빌리티가 각자 PauseLogic/ResumeLogic을 불러 돌진 해제가 그로기 중인 BT를 재개하던 문제를, 브레인 제어를 AWxAIController 하나로 모아 해결했다. 사망은 OnDeath → StopLogic, 그로기는 Ability.Groggy 태그 이벤트 → LockResource/ClearResourceLock(Reaction), 돌진은 브레인을 건드리지 않는다."
---

# AI 트리 정지·잠금을 AWxAIController 단독으로

2026-09-24 작업 트리. WxCombat 모듈 리뷰 1번(`.agents/workflow/tasks/module_review_WxCombat.md`) 후속. WxEditor Development 빌드 통과, 사용자 인게임 확인("테스트해봤는데 잘 되네요").

## 문제

- `UBrainComponent`의 일시정지는 단일 플래그다. Reason 문자열은 로그용이다(엔진 `BehaviorTreeComponent.cpp` `PauseLogic`·`ResumeLogic`).
- `UWxRootMotionModifier_Rush`와 `UWxAbility_Groggy`가 각자 `PauseLogic`/`ResumeLogic`을 불렀다. 돌진 중 그로기에 들면 돌진 modifier 해제가 `IsPaused()`만 보고 재개해 그로기 중인 BT가 다시 돌았다.

## 사용자 결정

- "UWxRootMotionModifier_Rush 관련해서는 Brain 안건드려도 될거 같아요."
- "Groggy 뿐만 아니라 Death도 AWxAIController 에서 처리하는게 낫지 않을까요?" → 사망은 이미 컨트롤러가 `OnDeath`로 처리 중임을 확인하고, 그로기도 컨트롤러로 옮기는 구성에 "네".

## 현재 구조

| 상태 | 컨트롤러가 받는 신호 | 트리 처리 |
| --- | --- | --- |
| 사망 | `AWxCharacterBase::OnDeath` | `StopLogic` |
| 그로기 | `Ability.Groggy` 태그 이벤트(`RegisterGameplayTagEvent`, NewOrRemoved) | `LockResource` / `ClearResourceLock(EAIRequestPriority::Reaction)` |
| 돌진 | 없음 | 관여하지 않는다 |

- 빙의 해제 시 구독을 끊으면서 Reaction 잠금도 푼다. 구독이 끊기면 태그 제거를 받지 못하기 때문이다.
- 그로기 어빌리티는 `ActivationOwnedTags`로 `Ability.Groggy`를 이미 붙이므로 새 신호를 만들지 않았다. 사망을 감지해 그로기를 끝내는 폴링은 전투 상태 정리라 유지한다.

## 판단 근거

- Rush가 브레인을 멈춘 이유는 2026-09-12 기록의 "기존 락온·분신 따라가기가 방향을 덮어쓰지 않게"였다. 따라가기 노드(`FollowMasterAbility` 등)는 `411e74d7f`(2026-09-23)에서 삭제됐다. 현재 돌진하는 AI는 Minion의 Skill_2뿐이고, `BT_Minion`은 Selector·Sequence·BeyondLeash·ObserveAbility·ActivateAbility만 쓴다(에셋 이름표 검색). 회전 간섭은 Rush가 `bUseControllerRotationYaw`·`bAllowPhysicsRotationDuringAnimRootMotion`으로 직접 막는다.
- Rush에서 제어를 빼면 `BT_Minion`의 ObserveAbility(LowerPriority) 중단이 돌진 구간에도 바로 적용된다. 이전에는 구간 끝까지 미뤄졌다. 사용자 인게임 확인에서 교차 돌진은 정상이었다.
- `PauseLogic` 대신 `LockResource`: 엔진은 잠금을 우선순위별 비트로 관리하고 모든 비트가 풀릴 때만 재개한다(`BrainComponent.cpp` `LockResource`·`ClearResourceLock`). 엔진 AI 태스크(`UAITask`, SmartObject·GameplayInteraction의 `bLockAILogic`)는 Logic 비트를 쓰므로(`AAIController::OnGameplayTaskResourcesClaimed`), 그 해제가 그로기 중 트리를 재개하지 않는다. `EAIRequestPriority::Reaction`은 엔진 정의상 피격 반응·사망처럼 AI 판단과 무관한 행동이다. 같은 우선순위 안에서는 계수하지 않으므로(`bUseResourceLockCount` 기본 false) Reaction은 컨트롤러만 건다.
- 잠금은 태그 변화에만 대칭으로 푼다. 엔진 `StopTree`·`StartTree`는 `bIsPaused`를 초기화하지 않는다. 사망 뒤 그로기 종료 때의 해제는 빈 트리에 무해하고, 재사용 트리가 멈춘 채 시작하는 것을 막는다.
- 태그 추가는 `PreActivate`에서 `Ability.*` 취소보다 먼저 일어나(`GameplayAbility.cpp` PreActivate의 `AddLooseGameplayTags` → `ApplyAbilityBlockAndCancelTags`) 취소로 끝난 BT 태스크가 다음 분기를 고르기 전에 잠긴다.

## 기각한 대안

- BT 데코레이터로 그로기 태그를 관찰해 분기를 멈춘다(리뷰 원안): 모든 BT 에셋에 같은 가드를 복제해야 한다. 순정 `UBTDecorator_CheckGameplayTagsOnActor`는 관찰자 중단을 지원하지 않는다(`bAllowAbort*` false).
- Rush 해제 때 `Ability.Groggy`·`Ability.Death`면 재개를 건너뛴다: 소유자가 늘면 재발하고, Rush에서 제어를 빼면 필요 없다.
- 그로기용 캐릭터 델리게이트 신설: 받는 쪽이 컨트롤러 하나라 태그 이벤트 직접 구독으로 충분하다.

## 남은 제약

- 트리 잠금은 이미 진행 중인 MoveTo 경로 추종과 포커스 회전을 멈추지 않는다(변경 전 `PauseLogic`도 같다). 그로기 몽타주에 루트 모션이 없어 미끄러짐이 보이면 `PathFollowingComponent`도 같은 Reaction 우선순위로 잠근다(엔진 `UPathFollowingComponent::LockResource` → `PauseMove`).

근거: [AI 컨트롤러](../../../Source/WxGame/Controller/WxAIController.cpp), [그로기 어빌리티](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp), [돌진 modifier](../../../Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp), [WxCombat 리뷰](../../../.agents/workflow/tasks/module_review_WxCombat.md).
