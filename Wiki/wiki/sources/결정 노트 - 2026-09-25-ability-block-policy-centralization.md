---
type: source
title: "결정 노트 - 2026-09-25-ability-block-policy-centralization"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "GAS"
  - "어빌리티"
summary: "어빌리티 자식 생성자의 공통 차단 코드를 지우고 ActivationGroup·태그 기반 계산을 ASC ApplyAbilityBlockAndCancelTags 확장 지점으로 통합한 결정과 검증 기록"
source_type: decision-note
source_id: src-1e7934105a2cce2c0778
sha256: 2b0fa3b07a5a305a90fdd6e257001c6b75ec9bfa3cd0614bff4fba6149286295
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-ability-block-policy-centralization.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-ability-block-policy-centralization.md"
raw_copy: ".raw/captured/2b0fa3b07a5a305a90fdd6e257001c6b75ec9bfa3cd0614bff4fba6149286295.md"
claim_ids:
  - clm-d9912a3acf-c1
  - clm-d9912a3acf-c2
  - clm-d9912a3acf-c3
  - clm-d9912a3acf-c4
key_claims:
  - "2026-09-25 사용자는 ASC의 ApplyAbilityBlockAndCancelTags 확장으로 공통 차단 목록을 계산해 Super에 전달하는 방식을 승인했다."
  - "이 작업에서 Exclusive·Override 발동 그룹은 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 공통으로 막고 Independent는 명시 목록만 쓴다."
  - "13개 자식 생성자의 AddExclusiveAbilityBlockTags 호출은 제거됐지만 어빌리티 자식 클래스 자체의 통합은 향후 희망으로만 남았다."
  - "이 변경은 빌드, 자동화 테스트 3개, GA 40개 차단 관계 1,600건 대조로 확인됐고 플레이·네트워크 동작은 미확인이다."
---

# 결정 노트 - 2026-09-25-ability-block-policy-centralization

- 원본: `.wiki/raw/notes/2026-09-25-ability-block-policy-centralization.md`
- 원자료 사본: `.raw/captured/2b0fa3b07a5a305a90fdd6e257001c6b75ec9bfa3cd0614bff4fba6149286295.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-25. 2026-09-25 미커밋 작업 트리 관찰이다.
- 앞선 <!--wl-->결정 노트 - 2026-09-25-exclusive-tag-blocking의 생성자 선언 방식을 대체한다. 콤보·도플갱어 정책은 유지하며, 승인·검증 상태의 정본은 작업 기록 `.agents/workflow/tasks/exclusive-tag-blocking.md`라고 노트는 적는다.
- 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-25: "네. 그렇게 합시다." (ASC의 ApplyAbilityBlockAndCancelTags 확장으로 계산한 목록을 Super에 전달하는 제안에 대해)

> 사용자 2026-09-25: "자식클래스를 최대한 축소하는걸 원해요", "자식클래스를 축소하려는 이유는 어빌리티의 공통 규칙이기 때문이에요"

> 사용자 2026-09-25: "이번에는 자식의 차단 코드만 제거"

> 사용자 2026-09-25: "나중에는 저런 자식클래스들도 없앨 수 있으면 없애고 싶어요"

마지막 발언은 향후 방향으로 기록됐고, 이번에 클래스 자체를 합치지는 않았다.

## 구현 계약(노트 시점)

- `UWxAbilityBase::GetAbilityBlockTags`는 명시한 `BlockAbilitiesWithTag`와 그룹 공통 규칙을 합친다. Independent는 명시 목록만, Exclusive·Override는 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 공통으로 막는다.
- Exclusive이며 `Ability.Attack.Light` 태그를 가지면 공통 Attack 부모 대신 Light·Air·DodgeCounter를 더한다. 강공격이 발동 검사를 통과한 뒤 순정 `CancelAbilitiesWithTag`로 약공격을 끊게 하는 관계다.
- 계산에 구체 자식 클래스 검사나 자식 재정의가 없어, 향후 자식 클래스 통합 때 그룹·태그 데이터만 보존되면 그대로 쓸 수 있다. 새 액션 태그 분류를 도입하면 공통 대상 목록과 Light 관계를 검토해야 한다.
- `UWxAbilitySystemComponent::ApplyAbilityBlockAndCancelTags`가 전달받은 BlockTags와 계산 결과를 합쳐 Super를 호출한다. 비 Wx 어빌리티·빈 요청자는 인자를 그대로 따르고, 취소 태그·순정 카운트 수명은 재구현하지 않는다.
- 발동·`SetShouldBlockOtherAbilities`·종료의 차단 적용/해제와 콤보 자기 기여 제외가 같은 계산을 쓴다. 목록은 실행 중 변하는 상태에 의존하면 안 되고 활성 수명 동안 고정이어야 한다.
- 13개 자식 생성자의 `AddExclusiveAbilityBlockTags` 호출과 Light의 차단 수정 코드를 제거했다. Death의 Ability 부모 차단은 명시 규칙으로 남는다.
- 에디터의 `BlockAbilitiesWithTag`는 개별 명시 선언이며 자동 공통 규칙을 에셋/CDO에 써 넣지 않는다.

## 검증 범위

- Editor Win64 Development 빌드 성공.
- 자동화 테스트 `Wx.Combat.AbilityBlocking.AssetDefaults`·`HookRules`·`Lifecycle` 성공 3, 실패 0(그룹·태그 계산, 명시 차단 보존, 비 Wx·빈 요청자, 콤보 기여 구분, GE 차단, 도플갱어 면제, 강공격 취소 등).
- 리다이렉트 없는 새 에디터 프로세스에서 GA 40개 CDO 계산 결과 대조: 차단 관계 1,600건, 점프 40건, 도플갱어 네 효과 참조 통과. 개별 명시 차단은 `GA_Shared_Death`의 Ability뿐이다.
- 렌더링·몽타주를 생략한 ServerOnly 테스트다. 사람의 코드 리뷰, 실제 입력·몽타주 타이밍, 도플갱어 BT 재시도, UI, 네트워크 예측/복제는 미확인이다.
- 엔진 수정 없이 UE 5.8 `GameplayAbility.cpp`·`AbilitySystemComponent_Abilities.cpp`의 순정 확장 지점을 대조했다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[플레이어 캐릭터와 조작]]

## 핵심 주장

- 2026-09-25 사용자는 ASC의 ApplyAbilityBlockAndCancelTags 확장으로 공통 차단 목록을 계산해 Super에 전달하는 방식을 승인했다. ^c1
- 이 작업에서 Exclusive·Override 발동 그룹은 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 공통으로 막고 Independent는 명시 목록만 쓴다. ^c2
- 13개 자식 생성자의 AddExclusiveAbilityBlockTags 호출은 제거됐지만 어빌리티 자식 클래스 자체의 통합은 향후 희망으로만 남았다. ^c3
- 이 변경은 빌드, 자동화 테스트 3개, GA 40개 차단 관계 1,600건 대조로 확인됐고 플레이·네트워크 동작은 미확인이다. ^c4
