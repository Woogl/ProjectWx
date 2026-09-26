---
type: source
title: "결정 노트 - 2026-09-24-wxcombat-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "전투"
  - "GAS"
  - "처형"
summary: "구간 GE 노티파이가 자기 핸들만 걷게 하고 처형 피해를 처형 어빌리티가 직접 적용하며 퍼펙트 가드 Cue를 Hit Cue로 통합한 WxCombat 정리 네 건 기록"
source_type: decision-note
source_id: src-b41b3e5cba0feec06842
sha256: 609caa7004bf69cc1ffe98e4b753533418d9d10afa8d41b926a0bff3e4953861
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-wxcombat-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-wxcombat-cleanup.md"
raw_copy: ".raw/captured/609caa7004bf69cc1ffe98e4b753533418d9d10afa8d41b926a0bff3e4953861.md"
claim_ids:
  - clm-ff8c959931-c1
  - clm-ff8c959931-c2
  - clm-ff8c959931-c3
  - clm-ff8c959931-c4
key_claims:
  - "커밋 ce6295184 이후 UWxAnimNotifyState_ApplyGameplayEffect는 몽타주 인스턴스 ID별로 자기가 건 GE 핸들의 스택 하나만 제거한다."
  - "사용자는 스택형이 아닌 GE도 겹치는 구간에 쓸 수 있도록 ANS가 자기 핸들만 제거하는 B안을 선택했다."
  - "커밋 4c1bf3e52 이후 처형 피해는 UWxFinisherDamageComponent 없이 UWxAbility_Finisher가 서버에서 Event.ApplyFinisherDamage를 받아 직접 적용한다."
  - "네 건의 WxCombat 정리는 인게임 플레이로 검증되지 않았다."
---

# 결정 노트 - 2026-09-24-wxcombat-cleanup

- 원본: `.wiki/raw/notes/2026-09-24-wxcombat-cleanup.md`
- 원자료 사본: `.raw/captured/609caa7004bf69cc1ffe98e4b753533418d9d10afa8d41b926a0bff3e4953861.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-24. 커밋 `ce6295184`·`4c1bf3e52`·`74fc58853`·`ca84c9aac`(2026-09-24), HEAD `ca84c9aac`에서 코드를 다시 읽어 대조했다.
- 노트 날짜 기준이라 현재 코드와 다를 수 있다. 플레이 검증은 네 건 모두 없다.

## 구간 GE 노티파이는 자기가 건 효과만 걷는다 (`ce6295184`)

- 문제(구현 관찰): `UWxAnimNotifyState_ApplyGameplayEffect`가 끝에서 `RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1)`로 같은 클래스 GE를 한 스택씩 걷어, 극한 회피·처형·컷신 무적이나 퍼펙트가드처럼 같은 GE 구간이 겹치면 다른 구간 효과까지 사라졌다. 피하려면 GE를 스택형으로 만들어야 했다.
- 확정 결정: 사용자가 "스택형이 아닌 GE도 쓸 수 있게" 하고 싶다며 ANS가 자기 핸들만 제거하는 B안을 골랐다. 경위는 [[작업 - nameplate-manager]]의 코드 리뷰 후속 절.

> 사용자 2026-09-24: "네 B로 합시다"

- 구조: 노티파이 객체는 몽타주 에셋에 하나라 여러 캐릭터가 공유하므로 구간을 몽타주 인스턴스 ID로 가른다. `TMap<int32, FActiveGameplayEffectHandle> AppliedEffects`(서버에서만 채움)에 핸들을 두고 끝에서 `RemoveActiveGameplayEffect(Handle, 1)`. 큐 경로는 `FAnimNotifyMontageInstanceContext`, 브랜칭 포인트 경로는 `BranchingPointNotifyBegin/End` 오버라이드로 ID를 받는다. 몽타주가 아닌 재생은 경고 후 적용하지 않는다.
- `UWxCombatLibrary::ApplyEffect`는 핸들을 반환한다. `UWxAbilityBase::EndAbility`의 `ActivationOwnedEffects` 제거와 `UWxSkillCutsceneComponent::Finish`의 컷신 무적 제거도 핸들의 스택 하나만 뺀다.
- 검증: 임시 자동화 테스트 `Wx.Combat.ApplyEffectNotify.OwnHandleOnly` 통과(겹침·브랜칭·비몽타주 경우). 사용자 지침에 따라 테스트 파일은 삭제하고 다시 빌드 통과를 확인했다. 플레이 미검증.

## 처형 피해는 처형 어빌리티가 직접 적용한다 (`4c1bf3e52`)

- 이전: `UWxFinisherDamageComponent`가 어빌리티가 쥔 대상·피해 행을 `BeginFinisherDamage`로 복사해 들고 `EndFinisherDamage`로 수명을 맞췄다.
- 현재(노트 시점): `UWxAbility_Finisher`가 서버에서만 `UAbilityTask_WaitGameplayEvent`로 `Event.ApplyFinisherDamage`를 한 번 기다리고, 발동 순간 대상 태그로 고른 변형(`bBackstab`)의 `DamageDataRow`로 `ApplyDamage`를 부른다. 피해 시점에 변형을 다시 고르지 않는다.
- `UWxAnimNotify_FinisherDamage`는 이벤트만 보낸다. 컴포넌트를 지우고 `BP_Template`(Player)·`BP_HGTest`를 재저장했다. 인게임 처형 피해 미검증.

## 퍼펙트 가드 Cue는 Hit Cue 클래스를 쓴다 (`ca84c9aac`)

- `UWxCueNotify_PerfectGuard`가 `UWxCueNotify_Hit`의 재생을 복제하고 있어, `GC_PerfectGuard` 부모를 `UWxCueNotify_Hit`로 바꾸고 `GameplayCueTag`를 `GameplayCue.PerfectGuard`로 지정한 뒤 전용 클래스를 지웠다. 발행 주체는 그대로다.
- 퍼펙트 가드 Cue도 이제 `CameraShake`를 지정할 수 있으나 `GC_PerfectGuard`의 실제 값은 확인하지 않았다.

## ExecCalc 캡처 정의 통합 (`74fc58853`)

- `FWxDamageBaseStatics`·`FWxDamageExecutionStatics`를 `FWxDamageStatics`와 `GetDamageStatics()` 하나로 합쳤다. 계산·판정 동작 변화 없음.

## 관련 주제

- [[피해 파이프라인]]
- [[그로기·경직·피니시]]
- [[어빌리티와 GAS]]

## 핵심 주장

- 커밋 ce6295184 이후 UWxAnimNotifyState_ApplyGameplayEffect는 몽타주 인스턴스 ID별로 자기가 건 GE 핸들의 스택 하나만 제거한다. ^c1
- 사용자는 스택형이 아닌 GE도 겹치는 구간에 쓸 수 있도록 ANS가 자기 핸들만 제거하는 B안을 선택했다. ^c2
- 커밋 4c1bf3e52 이후 처형 피해는 UWxFinisherDamageComponent 없이 UWxAbility_Finisher가 서버에서 Event.ApplyFinisherDamage를 받아 직접 적용한다. ^c3
- 네 건의 WxCombat 정리는 인게임 플레이로 검증되지 않았다. ^c4
