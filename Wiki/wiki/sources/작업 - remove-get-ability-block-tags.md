---
type: source
title: "작업 - remove-get-ability-block-tags"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - "source"
  - "작업-기록"
  - "GAS"
  - "어빌리티"
summary: "GetAbilityBlockTags와 ActivationGroup을 지우고 액션을 Ability.Action 태그 위치와 생성자의 순정 차단·취소 필드로 표현하며 콤보 진행·재생 속도·회피 방향을 정리한 2026-09-28~29 완료 작업 기록"
source_type: task-record
source_id: src-71b7745ce4472d64c885
sha256: 28306becff62e6edfd6b6eacc8c938c5189129316e4b04e78519e9bb0f6ec254
authority: primary
independence_key: ".agents/workflow/tasks/remove-get-ability-block-tags.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/remove-get-ability-block-tags.md"
raw_copy: ".raw/captured/28306becff62e6edfd6b6eacc8c938c5189129316e4b04e78519e9bb0f6ec254.md"
claim_ids:
  - clm-28306becff-c1
  - clm-28306becff-c2
  - clm-28306becff-c3
  - clm-28306becff-c4
  - clm-28306becff-c5
key_claims:
  - "2026-09-28 사용자 승인으로 UWxAbilityBase::GetAbilityBlockTags·ASC ApplyAbilityBlockAndCancelTags 재정의·EWxAbilityActivationGroup·ActivationGroup·CanBeCanceled 재정의가 삭제되고, 액션 태그가 Ability.Action 아래로 옮겨져 액션 판정은 GetAssetTags().HasTag(Ability.Action)이 됐다."
  - "2026-09-29(KST) 추가 변경 뒤 액션·반응 타입 생성자(약공격 포함)는 BlockAbilitiesWithTag에 Ability.Action 한 줄을, 사망은 Ability를 선언하며, 그로기의 취소 대상은 Ability.Action, 사망은 Ability.Action·Sprint·LockOn이다."
  - "베이스 DoesAbilitySatisfyTagRequirements는 재생 중인 액션을 CancelAbilitiesWithTag로 지목한 어빌리티에 그 액션의 차단 몫을 빼고 판정하고 자기 재발동은 콤보 창에서만 허용하며, 앞선 콤보 창 한정 규칙은 리슨 서버에서 선입력 강공격이 8회 모두 서버 거절되어 폐기됐다."
  - "같은 작업에서 콤보 진행이 UWxAbility_Combo로 올라가고, 재생 속도 기본값 1.0에 ASPD는 콤보 어빌리티만 따르며, 회피 방향은 베이스 PlayMontage의 가상 훅 SelectInputDirectionSection으로 처리하고, Ability.Action.Jump 태그는 삭제됐다."
  - "GetAbilityBlockTags 제거 작업은 빌드, GA 40개 CDO 규칙 대조, 에셋 19개 태그 이관, 헤드리스 전투 흐름·리슨 서버 임시 자동화로 AI가 확인했고 woogle이 2026-09-28 코드 리뷰와 화면 표시를 통과시켜 9/9로 완료됐다."
---

# 작업 - remove-get-ability-block-tags

- 원본: `.agents/workflow/tasks/remove-get-ability-block-tags.md`
- 원자료 사본: `.raw/captured/28306becff62e6edfd6b6eacc8c938c5189129316e4b04e78519e9bb0f6ec254.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 개요

`UWxAbilityBase::GetAbilityBlockTags`가 있으면 코드에서 Exclusive 어빌리티를 관리해야 한다는 사용자 문제 제기로 시작한 작업 기록이다. 처음에는 생성자에 계산 결과와 같은 목록을 선언하는 계획을 승인·구현·대조까지 했지만, 같은 날 "Exclusive 그룹 정책과 이중"이라는 문제 제기로 방향을 바꿔 ActivationGroup 자체를 없애고 액션을 `Ability.Action` 태그 위치로 표현했다. 상태는 완료(체크리스트 9/9 통과)다.

## 사람의 판단 원문

> 사용자 2026-09-28: "UWxAbilityBase::GetAbilityBlockTags() 라는 함수를 없앱시다. 이게 있으니까 코드에서 Exclusive 어빌리티를 관리해야되는 문제점이 있어요"

> 사용자 2026-09-28: "코드 품질이 중요해요", "EWxAbilityActivationGroup 개념도 없앨 수 있나요?" → 액션을 태그 위치로 표현하고 그룹을 없애는 AI 제안에 "네". 구현 승인: "네 승인합니다. 구현하세요"

> 사용자 2026-09-29(Q2): "네 일단 그렇게 하죠" → 피격 반응은 공격·스킬만 끊는 현행 유지

> 사용자 2026-09-29: "강공격 이슈는 WxAbility_Attack_Light의 적절한 가상함수를 override해서 해결할 수 있지 않을까요?" → 끼어드는 쪽 발동 판정 규칙 제안에 "내! 그렇게 해주세여"

> 사용자 2026-09-29: "WxAbility 관련 코드를 확인해서 더 품질을 올릴 수 있는 방법을 제안해주세요" → "1,2,3번 모두 여기서 진행합시다"

태그 이름은 사용자가 `Ability.Exclusive.*`·`Ability.Trait.Exclusive` 마커를 물었고, 식별 태그의 뜻과 슬롯 GA의 에셋 태그 덮어쓰기(마커 누락 위험)를 근거로 `Ability.Action`을 유지했다.

## 구현 결과

- 태그: 액션 9종과 자식(Attack.Light·Heavy·Air·DodgeCounter, Skill.1~4, Pattern.1~9, Ultimate, Dodge, Guard, UseItem, Interact)을 `Ability.Action.*`으로 옮기고 C++ 상수 이름도 맞췄다. `Ability.Action.Jump`는 작업 중 지워 점프 검사는 `Ability.Action` 차단을 본다. Sprint·LockOn·Passive·HitReact·GuardReact·Groggy·Death·Finisher·PlayMontageOnce는 그대로다. 에셋 19개는 임시 리다이렉트 ini로 재저장한 뒤 ini를 지웠다.
- 삭제: `GetAbilityBlockTags`, ASC `ApplyAbilityBlockAndCancelTags` 재정의, `EWxAbilityActivationGroup`·`ActivationGroup`, `CanBeCanceled` 재정의(Override 취소 면역). 옛 Exclusive 판정 자리(단계 전환·후딜 차단 해제·콤보 창·콤보 자기 재발동·후딜 액션 취소·입력 버퍼)는 `GetAssetTags().HasTag(Ability.Action)`이다.
- 차단·취소: 액션·반응 타입 생성자는 `BlockAbilitiesWithTag`에 `Ability.Action` 한 줄, 사망은 `Ability`. 그로기 취소 대상은 `Ability.Action`(그로기는 적 세트에만 있음), 사망은 `Ability.Action`·Sprint·LockOn이고 패시브는 빠졌다. 공격 4종은 식별 태그·소유 태그·차단을 직접 선언한다(`SetAttackTag` 헬퍼 제거).
- 끼어들기: 베이스 `DoesAbilitySatisfyTagRequirements`는 재생 중인 액션을 `CancelAbilitiesWithTag`로 지목한 어빌리티에 그 액션의 차단 몫을 빼고 보고(단계 무관), 자기 재발동은 콤보 창에서만 허용한다. 강공격은 약공격 본동작 중에도 언제든 나간다.
- 동작 변화: 반응이 발동해도 후딜 중인 액션을 따로 취소하지 않는다(피격 → 후딜 패턴은 몽타주 밀어냄으로 끝남, 가드 반응 → 후딜 가드는 끊지 않음, 처형·몽타주 재생 → 후딜 액션과 겹쳐 재생). 컷신 효과(`WxEffect_SkillCutscene`)는 `Ability.Action`·Sprint·LockOn을 막아 컷신 중 점프가 새로 막힌다.
- 품질 개선: 공격·스킬·패턴의 콤보 진행을 `UWxAbility_Combo`로 올렸다(창 닫힘 초기화는 입력 콤보인 공격·스킬에만). 재생 속도 기본값을 1.0으로 두고 ASPD는 콤보만 따른다(궁극기·아이템 사용은 더 이상 따르지 않음). 회피 방향은 베이스 `PlayMontage`의 가상 훅 `SelectInputDirectionSection`으로 처리하고 회피 자체의 방향 TargetData 송수신 코드(약 90줄)를 지웠다.

## 검증 범위

- AI: Editor Development 빌드(임시 테스트 삭제 뒤), GA 40개 CDO 옛 규칙 대조(차이는 새 부모 태그 자체의 차단뿐), 에셋 태그 이관(옛 태그 문자열 0), 헤드리스 전투 흐름 44항목·겹침 27항목, 추가 변경 25항목, 리슨 서버 강공격 5회 일치(서버 거절 0), 품질 개선 단독 25항목·리슨 서버 회피 방향 섹션 일치. ensure 0.
- 폐기된 안: 콤보 창 한정 취소 진입은 단독 17항목은 통과했으나 리슨 서버에서 본동작 중 누른 강공격이 8회 모두 서버에 거절됐다.
- 사람: woogle 2026-09-28 코드 리뷰·화면 표시(스킬·회피 아이콘, 상호작용 프롬프트, WBP_PlayerSkills·아이템 퀵슬롯) 통과.

## 기록 안의 서술 차이

기록의 「구현」 절은 약공격만 Heavy를 뺀 목록을 쓰고 그로기·사망의 취소 대상이 `Ability.Action`·Sprint·LockOn이라고 적지만, 같은 기록의 2026-09-29 추가 변경과 그 검증은 약공격 차단을 `Ability.Action` 한 줄로, 그로기 취소 대상을 `Ability.Action` 하나로 적는다. 이 요약은 뒤의 추가 변경과 체크리스트 근거를 따른다.

## 관련 주제

- [[어빌리티와 GAS]]

## 핵심 주장

- 2026-09-28 사용자 승인으로 UWxAbilityBase::GetAbilityBlockTags·ASC ApplyAbilityBlockAndCancelTags 재정의·EWxAbilityActivationGroup·ActivationGroup·CanBeCanceled 재정의가 삭제되고, 액션 태그가 Ability.Action 아래로 옮겨져 액션 판정은 GetAssetTags().HasTag(Ability.Action)이 됐다. ^c1
- 2026-09-29(KST) 추가 변경 뒤 액션·반응 타입 생성자(약공격 포함)는 BlockAbilitiesWithTag에 Ability.Action 한 줄을, 사망은 Ability를 선언하며, 그로기의 취소 대상은 Ability.Action, 사망은 Ability.Action·Sprint·LockOn이다. ^c2
- 베이스 DoesAbilitySatisfyTagRequirements는 재생 중인 액션을 CancelAbilitiesWithTag로 지목한 어빌리티에 그 액션의 차단 몫을 빼고 판정하고 자기 재발동은 콤보 창에서만 허용하며, 앞선 콤보 창 한정 규칙은 리슨 서버에서 선입력 강공격이 8회 모두 서버 거절되어 폐기됐다. ^c3
- 같은 작업에서 콤보 진행이 UWxAbility_Combo로 올라가고, 재생 속도 기본값 1.0에 ASPD는 콤보 어빌리티만 따르며, 회피 방향은 베이스 PlayMontage의 가상 훅 SelectInputDirectionSection으로 처리하고, Ability.Action.Jump 태그는 삭제됐다. ^c4
- GetAbilityBlockTags 제거 작업은 빌드, GA 40개 CDO 규칙 대조, 에셋 19개 태그 이관, 헤드리스 전투 흐름·리슨 서버 임시 자동화로 AI가 확인했고 woogle이 2026-09-28 코드 리뷰와 화면 표시를 통과시켜 9/9로 완료됐다. ^c5
