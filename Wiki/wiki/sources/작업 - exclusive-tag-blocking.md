---
type: source
title: "작업 - exclusive-tag-blocking"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - "source"
  - "작업-기록"
  - "GAS"
  - "어빌리티"
summary: "Exclusive 어빌리티 차단을 순정 태그 차단으로 옮긴 작업의 완료 기록으로, 2026-09-28 헤드리스·리슨 서버 테스트와 스킬 슬롯 아이콘 후보 선택 수정, 체크리스트 15/15 통과"
source_type: task-record
source_id: src-6b9d64b9441a771c418a
sha256: 8bea528318ccbbb1622af9d60c8bdd74638caf0a0f10c6c09bd3d6f66e21d563
authority: primary
independence_key: ".agents/workflow/tasks/exclusive-tag-blocking.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/exclusive-tag-blocking.md"
raw_copy: ".raw/captured/8bea528318ccbbb1622af9d60c8bdd74638caf0a0f10c6c09bd3d6f66e21d563.md"
claim_ids:
  - clm-8bea528318-c1
  - clm-8bea528318-c2
  - clm-8bea528318-c3
key_claims:
  - "2026-09-28 헤드리스 게임·리슨 서버 PIE 테스트에서 콤보·선입력, 반응 중첩·강공격 취소, 도플갱어 미러링, UI·상호작용 가용성은 통과했고, 서버만 콤보 재발동을 거절한 뒤의 client L / server LL 단계 어긋남은 combo-stage-desync-after-rejection 수정 뒤 재테스트로 통과했다."
  - "스킬 1 발동 뒤 아이콘이 스킬 2로 늦게 바뀌던 문제는 슬롯 후보 선택이 재생 중 액션 차단까지 본 탓이었고, 2026-09-29(KST) 수정으로 후보는 UWxAbilityBase::DoesOwnerSatisfyActivationTags로 고르고 WxGame 리졸버가 판정 대리자 FWxCanBindAbility를 넘기며 CanActivate 표시는 엔진 CanActivateAbility를 따른다."
  - "exclusive-tag-blocking 작업은 체크리스트 15/15 통과로 완료됐고 woogle이 2026-09-28 코드 리뷰와 화면 표시 모양을 통과시켰다."
---

# 작업 - exclusive-tag-blocking

- 원본: `.agents/workflow/tasks/exclusive-tag-blocking.md`
- 원자료 사본: `.raw/captured/8bea528318ccbbb1622af9d60c8bdd74638caf0a0f10c6c09bd3d6f66e21d563.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 개요

2026-09-25 Exclusive 어빌리티 발동 차단을 엔진 순정 태그 차단(`BlockAbilitiesWithTag`)으로 옮긴 작업의 완료 기록이다. 결정·구현 이력은 [[결정 노트 - 2026-09-25-exclusive-tag-blocking]]과 [[결정 노트 - 2026-09-25-ability-block-policy-centralization]]에 있고, 이 기록은 그 뒤 2026-09-28 헤드리스 테스트와 사람 확인, 스킬 슬롯 아이콘 수정까지 담는다. 이 작업의 공통 차단 함수(`GetAbilityBlockTags`)와 ActivationGroup은 뒤이은 [[작업 - remove-get-ability-block-tags]]에서 지워졌다. 상태는 완료(체크리스트 15/15 통과)다.

## 사람의 판단 원문

> 사용자 2026-09-25: "콤보 구간 동안에는 자기 재발동 허용되게 합시다."

> 사용자 2026-09-25: "네. 그렇게 진행해주세요. `IgnoreAbilityTags` 는 `IgnoreAbilityActivationTags` 로 하세요."

> 사용자 2026-09-28: "콤보는 일감 만들어주세요. 나중에 볼게요"

> woogle 2026-09-28(실패 전달): "BP_HGTest는 Ability.Action.Skill.1 발동 직후에 UI 표시되는 아이콘이 Ability.Action.Skill.2로 바로 바뀌지 않고 잠시 후에 행동해야 바뀌어요."

## 헤드리스 테스트(2026-09-28)

- 사람 항목 다섯 개를 AI 항목으로 옮겼다(`workflow-recheck.md` Q2). 단독은 LV_DevCombat `-game -nullrhi`에서 선입력 컴포넌트 → ASC 입력 경로로, 네트워크는 헤드리스 에디터 PIE(리슨 서버 + 원격 클라이언트, 한 프로세스)로 돌렸다.
- 콤보·선입력: 창 안 재입력은 LL로 이어지고, 본동작 재입력은 선입력으로 남았다가 창이 열리는 0.34초에 나간다. 창에서 누른 회피는 후딜 시작(0.42초)에 나가 공격을 취소한다. 매번 차단 태그가 남지 않는다.
- 반응 중첩: 강공격은 약공격을 취소하고, 가드 중 피격은 가드를 유지한 채 가드 반응으로, 공격 중 피격은 Normal 반응으로 들어간다. 패턴 중 그로기·사망은 패턴을 취소하고, 그로기 적 앞 상호작용으로 처형이 나간다.
- 도플갱어: 분신이 주인의 L→LLLL을 같은 몽타주로 따라 쓰고, 컷신 GE·Ability 전체 차단 중에는 재시도가 0.401초에 만료되어 따라 쓰지 않는다.
- UI·상호작용 가용성: 본동작·콤보 창에서는 회피·스킬 CanActivate가 false이고 상호작용 목록이 0개, 후딜·종료에서는 true·1개였다.
- 실패 1건: 서버만 콤보 재발동을 거절하면 다음 공격이 client L / server LL로 어긋났다. 수정은 `combo-stage-desync-after-rejection.md` 일감에서 했고 재테스트로 통과했다([[작업 - combo-stage-desync-after-rejection]]). 기록의 다음 행동은 커밋 전에 그 코드 리뷰를 함께 확인하라고 적는다.

## 스킬 슬롯 아이콘 수정(2026-09-29 KST)

- 원인: 스킬 슬롯 VM은 같은 슬롯 후보 중 표시할 어빌리티를 `DoesAbilitySatisfyTagRequirements`로 골랐다. 차단이 순정 BlockedAbilityTags로 옮겨지면서 이 판정에 재생 중인 스킬 1의 Ability.Action 차단이 들어가, 분신이 생겨 스킬 2가 요건을 채워도 후보에서 빠졌다.
- 수정: 후보는 소유자 태그만으로 본 발동 조건(`UWxAbilityBase::DoesOwnerSatisfyActivationTags`)으로 고른다. WxUI는 WxCombat을 모르므로 `UWxViewModel_Ability`에 판정 대리자 `FWxCanBindAbility`를 두고 WxGame `WxViewModelResolver_Ability`가 넘긴다. 사용 가능 표시(CanActivate)는 엔진 CanActivateAbility 그대로다.
- 헤드리스 재현: 수정 전 분신 0.157초·아이콘 교체 0.821초(Fail), 수정 후 분신 0.158초·아이콘 교체 0.167초(Success). 그동안 스킬 2는 차단 상태로 남았다.

## 검증 범위

- AI: 빌드(임시 테스트 삭제 뒤), GA 40개 차단 관계 1,600건, 효과 이름·에셋 참조, 임시 자동화 수명·규칙 테스트, 위 헤드리스·네트워크 테스트, 슬롯 아이콘 테스트.
- 사람: woogle 2026-09-28 코드 리뷰·화면 표시 모양 통과.
- 이 갱신은 기록을 수집했을 뿐 게임을 다시 실행하지 않았다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[UI 표시 구조]]
- [[적 AI와 몬스터]]

## 핵심 주장

- 2026-09-28 헤드리스 게임·리슨 서버 PIE 테스트에서 콤보·선입력, 반응 중첩·강공격 취소, 도플갱어 미러링, UI·상호작용 가용성은 통과했고, 서버만 콤보 재발동을 거절한 뒤의 client L / server LL 단계 어긋남은 combo-stage-desync-after-rejection 수정 뒤 재테스트로 통과했다. ^c1
- 스킬 1 발동 뒤 아이콘이 스킬 2로 늦게 바뀌던 문제는 슬롯 후보 선택이 재생 중 액션 차단까지 본 탓이었고, 2026-09-29(KST) 수정으로 후보는 UWxAbilityBase::DoesOwnerSatisfyActivationTags로 고르고 WxGame 리졸버가 판정 대리자 FWxCanBindAbility를 넘기며 CanActivate 표시는 엔진 CanActivateAbility를 따른다. ^c2
- exclusive-tag-blocking 작업은 체크리스트 15/15 통과로 완료됐고 woogle이 2026-09-28 코드 리뷰와 화면 표시 모양을 통과시켰다. ^c3
