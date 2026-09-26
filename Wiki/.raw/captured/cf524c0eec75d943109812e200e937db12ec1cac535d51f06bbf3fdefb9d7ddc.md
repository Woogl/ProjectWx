---
title: "Exclusive 태그 차단과 도플갱어 발동 조건 면제"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, combat, ai, decision]
summary: "Exclusive 발동 차단을 순정 BlockAbilitiesWithTag로 옮겼다. 콤보는 자기 차단 기여만 제외하고, 도플갱어의 IgnoreAbilityActivationTags는 소유자 발동 조건만 면제한다. 빌드·GAS 회귀 2개·GA 40개의 차단 관계 1,600건·리다이렉트 없는 효과 참조 로드를 확인했다. 플레이·네트워크는 미확인이다."
---

# Exclusive 태그 차단과 도플갱어 발동 조건 면제

2026-09-25 미커밋 작업 트리 관찰이다. 구현 승인·사람의 코드 리뷰·플레이 확인은 [작업 기록](../../../.agents/workflow/tasks/exclusive-tag-blocking.md)에서 구분한다.

## 사용자 결정

- "Exclusive 어빌리티의 발동 차단을 태그 방식으로 수정할까요? 엔진의 순정 규칙을 따르는게 나을 것 같은 느낌이 들어서요"
- "콤보 구간 동안에는 자기 재발동 허용되게 합시다. 도플갱어 같은 경우는 어떻게 해야할까요?"
- "네. 그렇게 진행해주세요. `IgnoreAbilityTags` 는 `IgnoreAbilityActivationTags` 로 하세요."

승인 범위는 순정 태그 차단 전환, 콤보 자기 재발동 유지, 도플갱어 면제 범위 축소와 효과 이름·에셋 참조 이관이다. 비용·쿨다운은 기존 별도 면제 효과를 유지한다.

## 구현 관찰

- 베이스의 `FindActivationGroupBlocker`와 `CanActivateAbility` 재정의를 제거했다. 일반 액션과 반응 타입 생성자의 `AddExclusiveAbilityBlockTags`가 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 `BlockAbilitiesWithTag`에 더한다. 반응·처형 태그는 이 목록에 없어 진입할 수 있다. Death는 기존 Ability 부모 차단을 유지한다.
- `ActivationGroup`은 입력 버퍼 분류·단계 전이·Override 취소 면역에 남는다. 그룹만 바꿔서 차단 선언이 자동 갱신되지는 않는다. 새로운 액션 분류와 공격 하위 타입을 도입하면 공통 목록과 Light의 개별 목록을 함께 검토해야 한다.
- 엔진은 발동 검사 이후 `PreActivate`에서 차단·취소를 적용한다. Light가 Attack 부모 대신 Light·Air·DodgeCounter를 막아 Heavy의 진입을 허용하고, Heavy의 `CancelAbilitiesWithTag`가 Light를 끝낸다.
- ComboWindow에서 활성 인스턴스 자신의 차단 기여 1건만 제외한다. `UWxAbilitySystemComponent::AreAbilityTagsBlockedIgnoringContribution`은 명시 태그의 직접 등록 횟수(`GetExplicitTagCount`)를 비교한다. 부모 집계 횟수를 쓰거나 가용성 조회 중 카운터를 수정하지 않는다. 같은 태그의 다른 스펙·외부 어빌리티·GE 차단은 남는다.
- 콤보 다음 단에서 소유자 ActivationRequired/Blocked 조건을 면제하는 기존 규칙을 보존한다. 일반 첫 발동은 순정 조건을 검사한다. 전달된 Source·Target 조건은 콤보와 도플갱어 모두 검사한다.
- Recovery는 순정 `SetShouldBlockOtherAbilities(false)`로 자기 차단만 해제한다. 다음 배타 발동/점프가 이전 후딜을 실제로 끝내는 `CancelRecoveringAbilities`는 유지한다. 점프는 GA를 만들지 않고 Ability.Jump 차단을 조회한다.
- `UWxEffect_IgnoreAbilityActivationTags`는 Effect.IgnoreAbilityActivationTags를 부여한다. 도플갱어의 소유자 발동 조건만 면제하며 자기 본동작·콤보·후딜, 어빌리티·GE 차단은 지킨다. 이전 효과의 모든 태그 검사 우회를 좁힌 것이다.
- `ABS_Doppelganger`의 네 효과는 IgnoreAggro·IgnoreCooldowns·IgnoreCosts·IgnoreAbilityActivationTags다. 임시 ClassRedirect로 에셋을 저장한 뒤 리다이렉트를 제거했다.
- 미러링 태스크는 변경하지 않았다. 원본 클래스·레벨을 자기 ASC에 부여하고 TryActivateAbility로 실행한다. RetryDuration 기본값은 0.4초이고 최신 실패 요청 하나만 재시도한다. 임의 지연과 다중 요청의 완전 동기화를 보장하지 않는다.

## 검증과 한계

- Editor Win64 Development 빌드 성공. 최종 테스트 구성 빌드 로그: `Saved/Logs/BuildDoctor/build_2026-09-25_223726_639_47172.log` (`Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`).
- 새 에디터 프로세스에서 리다이렉트 없이 GA 40개 CDO의 차단 관계 1,600건과 점프 차단 40건, 도플갱어 네 효과 참조를 대조했다. `Saved/Logs/ExclusiveTagValidation.log`, `Saved/Tests/ExclusiveAbilityAssets.json`.
- `Wx.Combat.AbilityBlocking.AssetDefaults`·`Lifecycle` 성공 2, 실패 0, 경고 0. `Saved/Tests/ExclusiveAbilityBlocking/index.json`, `Saved/Logs/ExclusiveAbilityBlocking-Retry.log`.
- 수명 테스트는 실제 공격 타입을 상속한 에디터 전용 테스트 GA와 실제 WxASC·전투 속성 세트·면제/컷신 GE를 사용한다. 자기 콤보와 동일 태그의 다른 스펙 구분, 동일 태그의 추가 차단, 부모 태그 GE 차단, 소유자 조건 면제, Source 조건 검사, 순정 재발동, Light→Heavy 취소, Recovery 교체, 취소·즉시 종료의 차단 정리를 확인했다.
- 첫 수명 테스트는 테스트용 ASC의 속성 세트 누락으로 비용 검사에서 중단됐다. 속성 세트를 추가해 재빌드·재실행한 결과가 위 성공 기록이다. 게임 코드 결함으로 분류하지 않는다.
- 몽타주를 생략한 ServerOnly 테스트이므로 실제 노티파이/선입력 감각, 도플갱어 BT 재시도 타이밍, UI 갱신, 서버 발동 거절·예측/복제는 검증하지 않았다. 사람의 코드 리뷰와 플레이 수용도 완료되지 않았다.

## 코드·에셋 근거

- [AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp)의 생성자 보조 함수, SetActionPhase, DoesAbilitySatisfyTagRequirements, ActivateAbility.
- [ASC](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp)의 차단 기여 제외 조회·CancelRecoveringAbilities.
- [공격 타입](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp)의 Light/Heavy 선언.
- [효과](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_IgnoreAbilityActivationTags.cpp)와 [캐릭터 점프](../../../Source/WxGame/Character/WxCharacterBase.cpp).
- [미러링 태스크](../../../Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp)와 [RetryDuration 기본값](../../../Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h).
- 로컬 UE 5.8.2 GameplayAbility.cpp의 DoesAbilitySatisfyTagRequirements·PreActivate·SetShouldBlockOtherAbilities·EndAbility, AbilitySystemComponent_Abilities.cpp의 발동·차단 카운트 처리와 대조했다.

기록 시 HEAD: `c9e2efec6b9354b1d18c572fca429b64022d7ddc`. 다른 작업의 변경을 포함할 수 있으므로 아래 파일 해시로 입력을 식별한다.

| 파일 | SHA-256 |
|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp` | `9e45dda7f91072be4890fdef0a3eb599eaf01773b45ed32d3537c2e029286319` |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp` | `b12057ed34c0aff40005bf5f57b45d0b77648a26385963e472fc3fb8136adca3` |
| `Source/WxEditor/Tests/WxAbilityBlockingTests.cpp` | `d6d963c26cce42437162c7cfd549417bb358efd95a2340ec36c2d309166054e7` |
| `Content/Character/Doppelganger/ABS_Doppelganger.uasset` | `3536f9181ed93fb62fbe0bebeedd1d81aa91cc0e37c0e7c8e79de507d6c2c1e2` |
