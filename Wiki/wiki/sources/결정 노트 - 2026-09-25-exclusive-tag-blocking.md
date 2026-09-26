---
type: source
title: "결정 노트 - 2026-09-25-exclusive-tag-blocking"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "GAS"
  - "도플갱어"
summary: "Exclusive 발동 차단을 순정 BlockAbilitiesWithTag로 옮기고 콤보 자기 재발동과 도플갱어 소유자 발동 조건 면제 범위를 정한 결정과 검증"
source_type: decision-note
source_id: src-4b6f12707fdf98c013f3
sha256: cf524c0eec75d943109812e200e937db12ec1cac535d51f06bbf3fdefb9d7ddc
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-exclusive-tag-blocking.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-exclusive-tag-blocking.md"
raw_copy: ".raw/captured/cf524c0eec75d943109812e200e937db12ec1cac535d51f06bbf3fdefb9d7ddc.md"
claim_ids:
  - clm-f1cb443cff-c1
  - clm-f1cb443cff-c2
  - clm-f1cb443cff-c3
  - clm-f1cb443cff-c4
key_claims:
  - "2026-09-25 사용자 승인으로 Exclusive 어빌리티 발동 차단이 커스텀 CanActivateAbility 재정의에서 순정 BlockAbilitiesWithTag 선언으로 옮겨졌다."
  - "ComboWindow 동안에는 활성 인스턴스 자신의 차단 기여 1건만 제외해 자기 재발동을 허용하고 다른 스펙·외부 어빌리티·GE 차단은 유지한다."
  - "도플갱어 면제 효과는 사용자 지시로 IgnoreAbilityActivationTags로 이름이 바뀌고 소유자 발동 조건만 면제하도록 범위가 좁혀졌다."
  - "Exclusive 태그 차단은 빌드·GA 40개 차단 관계 대조·자동화 테스트 2건으로 확인되었으나 플레이·예측/복제·사람 코드 리뷰는 미확인이다."
---

# 결정 노트 - 2026-09-25-exclusive-tag-blocking

- 원본: `.wiki/raw/notes/2026-09-25-exclusive-tag-blocking.md`
- 원자료 사본: `.raw/captured/cf524c0eec75d943109812e200e937db12ec1cac535d51f06bbf3fdefb9d7ddc.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-exclusive-tag-blocking.md`(제목 "Exclusive 태그 차단과 도플갱어 발동 조건 면제", source `MANUAL`, ingested 2026-09-25)를 요약한다. 2026-09-25 미커밋 작업 트리 관찰이며 기록 시 HEAD는 `c9e2efec6b9354b1d18c572fca429b64022d7ddc`다. 구현 승인·코드 리뷰·플레이 확인은 작업 기록 `.agents/workflow/tasks/exclusive-tag-blocking.md`에서 구분한다고 적혀 있다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-25: "Exclusive 어빌리티의 발동 차단을 태그 방식으로 수정할까요? 엔진의 순정 규칙을 따르는게 나을 것 같은 느낌이 들어서요"
> 사용자 2026-09-25: "콤보 구간 동안에는 자기 재발동 허용되게 합시다. 도플갱어 같은 경우는 어떻게 해야할까요?"
> 사용자 2026-09-25: "네. 그렇게 진행해주세요. `IgnoreAbilityTags` 는 `IgnoreAbilityActivationTags` 로 하세요."

승인 범위: 순정 태그 차단 전환, 콤보 자기 재발동 유지, 도플갱어 면제 범위 축소와 효과 이름·에셋 참조 이관. 비용·쿨다운은 기존 별도 면제 효과를 유지한다.

## 구현 관찰

- 베이스의 `FindActivationGroupBlocker`와 `CanActivateAbility` 재정의를 제거했다. 일반 액션·반응 타입 생성자의 `AddExclusiveAbilityBlockTags`가 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 `BlockAbilitiesWithTag`에 더한다. 반응·처형 태그는 목록에 없어 진입할 수 있고, Death는 Ability 부모 차단을 유지한다.
- `ActivationGroup`은 입력 버퍼 분류·단계 전이·Override 취소 면역에 남는다. 그룹만 바꿔서 차단 선언이 자동 갱신되지 않으므로 새 액션 분류·공격 하위 타입 도입 시 공통 목록과 Light 개별 목록을 함께 검토해야 한다.
- Light는 Attack 부모 대신 Light·Air·DodgeCounter를 막아 Heavy 진입을 허용하고, Heavy의 `CancelAbilitiesWithTag`가 Light를 끝낸다.
- ComboWindow에서는 활성 인스턴스 자신의 차단 기여 1건만 제외한다. `UWxAbilitySystemComponent::AreAbilityTagsBlockedIgnoringContribution`이 `GetExplicitTagCount`로 직접 등록 횟수를 비교하며, 다른 스펙·외부 어빌리티·GE 차단은 남는다.
- 콤보 다음 단의 소유자 ActivationRequired/Blocked 면제 규칙은 보존하고, Source·Target 조건은 콤보·도플갱어 모두 검사한다.
- Recovery는 순정 `SetShouldBlockOtherAbilities(false)`로 자기 차단만 해제하며 `CancelRecoveringAbilities`는 유지한다. 점프는 GA 없이 Ability.Jump 차단을 조회한다.
- `UWxEffect_IgnoreAbilityActivationTags`는 도플갱어의 소유자 발동 조건만 면제한다(이전 효과의 모든 태그 검사 우회를 좁힘). `ABS_Doppelganger`의 네 효과는 IgnoreAggro·IgnoreCooldowns·IgnoreCosts·IgnoreAbilityActivationTags이며, 임시 ClassRedirect로 저장 후 리다이렉트를 제거했다.
- 미러링 태스크(`WxBTTask_MirrorAbility`)는 변경하지 않았다. RetryDuration 기본 0.4초, 최신 실패 요청 하나만 재시도하며 임의 지연·다중 요청의 완전 동기화는 보장하지 않는다.

## 검증 범위

- Editor Win64 Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-25_223726_639_47172.log`).
- 새 에디터 프로세스에서 리다이렉트 없이 GA 40개 CDO의 차단 관계 1,600건, 점프 차단 40건, 도플갱어 네 효과 참조를 대조했다.
- `Wx.Combat.AbilityBlocking.AssetDefaults`·`Lifecycle` 성공 2, 실패 0. 첫 수명 테스트는 테스트용 ASC의 속성 세트 누락으로 중단됐고 게임 코드 결함으로 분류하지 않았다.
- 몽타주를 생략한 ServerOnly 테스트이므로 실제 노티파이·선입력 감각, 도플갱어 BT 재시도 타이밍, UI, 서버 발동 거절·예측/복제는 검증하지 않았다. 사람의 코드 리뷰와 플레이 수용도 완료되지 않았다.
- 엔진 대조는 로컬 UE 5.8.2 `GameplayAbility.cpp`·`AbilitySystemComponent_Abilities.cpp` 정적 읽기다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[적 AI와 몬스터]]
- [[결정 노트 - 2026-09-25-exclusive-submission-cleanup]]
- [[결정 노트 - 2026-09-25-ability-block-policy-centralization]]

## 핵심 주장

- 2026-09-25 사용자 승인으로 Exclusive 어빌리티 발동 차단이 커스텀 CanActivateAbility 재정의에서 순정 BlockAbilitiesWithTag 선언으로 옮겨졌다. ^c1
- ComboWindow 동안에는 활성 인스턴스 자신의 차단 기여 1건만 제외해 자기 재발동을 허용하고 다른 스펙·외부 어빌리티·GE 차단은 유지한다. ^c2
- 도플갱어 면제 효과는 사용자 지시로 IgnoreAbilityActivationTags로 이름이 바뀌고 소유자 발동 조건만 면제하도록 범위가 좁혀졌다. ^c3
- Exclusive 태그 차단은 빌드·GA 40개 차단 관계 대조·자동화 테스트 2건으로 확인되었으나 플레이·예측/복제·사람 코드 리뷰는 미확인이다. ^c4
