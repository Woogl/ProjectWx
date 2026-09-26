---
type: source
title: "결정 노트 - 2026-09-24-ai-brain-control-single-owner"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "AI"
  - "그로기"
  - "구조"
summary: "AI 비헤이비어 트리 정지·잠금을 AWxAIController 하나로 모은 결정. 사망은 StopLogic, 그로기는 Reaction 우선순위 리소스 잠금, 돌진은 브레인을 건드리지 않는다."
source_type: decision-note
source_id: src-49b82efe2fd5b598b233
sha256: fe0ebd409022031b8061ab8eef7c17a7c655525d0b8c19eccc9486a3943c7301
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-ai-brain-control-single-owner.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-ai-brain-control-single-owner.md"
raw_copy: ".raw/captured/fe0ebd409022031b8061ab8eef7c17a7c655525d0b8c19eccc9486a3943c7301.md"
claim_ids:
  - clm-01afeb75d2-c1
  - clm-01afeb75d2-c2
  - clm-01afeb75d2-c3
  - clm-01afeb75d2-c4
key_claims:
  - "2026-09-24 이후 WX AI의 비헤이비어 트리 정지·잠금은 AWxAIController만 담당하고 UWxRootMotionModifier_Rush는 브레인을 건드리지 않는다."
  - "AWxAIController는 Ability.Groggy 태그 이벤트에 LockResource와 ClearResourceLock(EAIRequestPriority::Reaction)으로 대응하고 사망에는 StopLogic으로 대응한다."
  - "AI 브레인 제어 단일화 변경은 WxEditor 빌드를 통과했고 사용자가 인게임에서 교차 돌진을 포함해 확인했다."
  - "그로기 중 진행 중인 MoveTo 경로 추종은 트리 잠금으로 멈추지 않으며 PathFollowingComponent 잠금은 필요 시 추가할 대안으로 남아 있다."
---

# 결정 노트 - 2026-09-24-ai-brain-control-single-owner

- 원본: `.wiki/raw/notes/2026-09-24-ai-brain-control-single-owner.md`
- 원자료 사본: `.raw/captured/fe0ebd409022031b8061ab8eef7c17a7c655525d0b8c19eccc9486a3943c7301.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-24-ai-brain-control-single-owner.md` (source: MANUAL, ingested: 2026-09-24).
- 2026-09-24 작업 트리 기준, WxCombat 모듈 리뷰 1번(`.agents/workflow/tasks/module_review_WxCombat.md`)의 후속이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.
- WxEditor Development 빌드 통과, 사용자 인게임 확인.

## 문제(구현 관찰)

- `UBrainComponent`의 일시정지는 단일 플래그이며 Reason 문자열은 로그용이다.
- `UWxRootMotionModifier_Rush`와 `UWxAbility_Groggy`가 각자 `PauseLogic`/`ResumeLogic`을 불러, 돌진 중 그로기에 들면 돌진 modifier 해제가 `IsPaused()`만 보고 재개해 그로기 중인 BT가 다시 돌았다.

## 사람의 판단 원문

> 사용자 2026-09-24: "UWxRootMotionModifier_Rush 관련해서는 Brain 안건드려도 될거 같아요."
> 사용자 2026-09-24: "Groggy 뿐만 아니라 Death도 AWxAIController 에서 처리하는게 낫지 않을까요?"

- 사망은 이미 컨트롤러가 `OnDeath`로 처리 중임을 확인하고 그로기도 컨트롤러로 옮기는 구성을 제시하자 사용자가 "네"로 승인했다.

> 사용자 2026-09-24(인게임 확인): "테스트해봤는데 잘 되네요"

## 확정 구조

| 상태 | 컨트롤러가 받는 신호 | 트리 처리 |
| --- | --- | --- |
| 사망 | `AWxCharacterBase::OnDeath` | `StopLogic` |
| 그로기 | `Ability.Groggy` 태그 이벤트(NewOrRemoved) | `LockResource` / `ClearResourceLock(EAIRequestPriority::Reaction)` |
| 돌진 | 없음 | 관여하지 않는다 |

- 빙의 해제 시 구독을 끊으면서 Reaction 잠금도 푼다. 그로기 어빌리티는 `ActivationOwnedTags`로 `Ability.Groggy`를 이미 붙이므로 새 신호를 만들지 않았다.

## 판단 근거

- Rush가 브레인을 멈춘 이유(락온·분신 따라가기 방향 덮어쓰기 방지)는 따라가기 노드가 `411e74d7f`(2026-09-23)에서 삭제되며 사라졌다. 현재 돌진하는 AI는 Minion의 Skill_2뿐이고, 회전 간섭은 Rush가 직접 막는다.
- Rush에서 제어를 빼면 `BT_Minion`의 ObserveAbility(LowerPriority) 중단이 돌진 구간에도 바로 적용된다. 사용자 인게임 확인에서 교차 돌진은 정상이었다.
- `PauseLogic` 대신 `LockResource`: 엔진은 잠금을 우선순위별 비트로 관리하고 모든 비트가 풀릴 때만 재개한다. 엔진 AI 태스크는 Logic 비트를 쓰므로 그 해제가 그로기 중 트리를 재개하지 않는다. 같은 우선순위 안에서는 계수하지 않으므로 Reaction은 컨트롤러만 건다.
- 잠금은 태그 변화에만 대칭으로 푼다(엔진 `StopTree`·`StartTree`는 `bIsPaused`를 초기화하지 않음).
- 태그 추가가 `PreActivate`에서 `Ability.*` 취소보다 먼저 일어나, 취소로 끝난 BT 태스크가 다음 분기를 고르기 전에 잠긴다.

## 기각한 대안

- BT 데코레이터로 그로기 태그를 관찰(리뷰 원안): 모든 BT에 가드를 복제해야 하고 순정 `UBTDecorator_CheckGameplayTagsOnActor`는 관찰자 중단을 지원하지 않는다.
- Rush 해제 때 그로기·사망이면 재개를 건너뛰기: 소유자가 늘면 재발한다.
- 그로기용 캐릭터 델리게이트 신설: 받는 쪽이 컨트롤러 하나라 불필요.

## 미결정·충돌(남은 제약)

- 트리 잠금은 진행 중인 MoveTo 경로 추종과 포커스 회전을 멈추지 않는다. 그로기 몽타주에 루트 모션이 없어 미끄러짐이 보이면 `PathFollowingComponent`도 같은 Reaction 우선순위로 잠그는 안이 남아 있다.

## 관련 주제

- [[적 AI와 몬스터]]
- [[그로기·경직·피니시]]
- [[모듈 구조와 코드 정리]]

## 핵심 주장

- 2026-09-24 이후 WX AI의 비헤이비어 트리 정지·잠금은 AWxAIController만 담당하고 UWxRootMotionModifier_Rush는 브레인을 건드리지 않는다. ^c1
- AWxAIController는 Ability.Groggy 태그 이벤트에 LockResource와 ClearResourceLock(EAIRequestPriority::Reaction)으로 대응하고 사망에는 StopLogic으로 대응한다. ^c2
- AI 브레인 제어 단일화 변경은 WxEditor 빌드를 통과했고 사용자가 인게임에서 교차 돌진을 포함해 확인했다. ^c3
- 그로기 중 진행 중인 MoveTo 경로 추종은 트리 잠금으로 멈추지 않으며 PathFollowingComponent 잠금은 필요 시 추가할 대안으로 남아 있다. ^c4
