---
title: "어빌리티 공통 차단 규칙의 ASC 확장 지점 통합"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, combat, architecture, decision]
summary: "자식 생성자의 공통 차단 코드를 제거하고 ActivationGroup·에셋 태그 기반 계산과 순정 ASC ApplyAbilityBlockAndCancelTags로 통합했다. 빌드·GAS 회귀 3개·GA 40개 차단 관계 1,600건을 확인했다. 클래스 자체의 통합은 향후 검토이며 플레이·네트워크는 미확인이다."
---

# 어빌리티 공통 차단 규칙의 ASC 확장 지점 통합

2026-09-25 미커밋 작업 트리 관찰이다. [앞선 태그 차단 전환](2026-09-25-exclusive-tag-blocking.md)의 생성자 선언 방식을 대체한다. 콤보·도플갱어 정책은 유지하며 [작업 기록](../../../.agents/workflow/tasks/exclusive-tag-blocking.md)이 승인·검증 상태의 정본이다.

## 사용자 결정과 범위

- ASC의 ApplyAbilityBlockAndCancelTags 확장으로 계산한 목록을 Super에 전달하는 제안에 "네. 그렇게 합시다."라고 승인했다.
- "자식클래스를 최대한 축소하는걸 원해요", "자식클래스를 축소하려는 이유는 어빌리티의 공통 규칙이기 때문이에요"라고 목적을 설명했다.
- 범위 질문에 "이번에는 자식의 차단 코드만 제거"라고 답했다.
- "나중에는 저런 자식클래스들도 없앨 수 있으면 없애고 싶어요"는 향후 방향으로 기록했다. 이번에 클래스 자체를 합치지는 않았다.

## 구현 계약

- UWxAbilityBase::GetAbilityBlockTags는 명시한 BlockAbilitiesWithTag와 그룹 공통 규칙을 합친다. Independent는 명시 목록만 사용한다. Exclusive·Override는 Attack·Skill·Pattern·Ultimate·Dodge·Guard·UseItem·Interact·Jump를 공통으로 막는다.
- Exclusive이며 Ability.Attack.Light 태그를 가진 경우 공통 Attack 부모 대신 Light·Air·DodgeCounter를 더한다. 강공격이 발동 검사를 통과한 뒤 순정 CancelAbilitiesWithTag로 약공격을 끊도록 하는 관계다. 명시한 차단 목록은 보존하므로 직접 선언한 Attack 부모까지 제거하지 않는다.
- 계산에는 구체 자식 클래스 검사나 자식 재정의가 없다. 향후 자식 클래스 통합 시 그룹·태그 데이터와 고유 동작이 보존되면 이 공통 규칙을 그대로 사용할 수 있다. 새 액션 태그 분류를 도입하면 공통 대상 목록과 Light 관계를 검토해야 한다.
- UWxAbilitySystemComponent::ApplyAbilityBlockAndCancelTags가 전달받은 BlockTags와 계산 결과를 합쳐 Super를 호출한다. 네이티브 비 Wx 어빌리티와 빈 요청자는 전달 인자를 그대로 따른다. 취소 태그나 순정 카운트 수명을 재구현하지 않는다.
- 발동·SetShouldBlockOtherAbilities·종료의 차단 적용/해제와 콤보 자기 기여 제외가 동일한 계산을 사용한다. 목록은 실행 중 변하는 액션 단계·체력·월드 상태에 의존하면 안 되며, 그룹·식별 태그·명시 차단 선언은 활성 수명 동안 고정해야 한다.
- 13개 자식 생성자의 AddExclusiveAbilityBlockTags 호출과 Light의 차단 수정 코드를 제거했다. Death의 Ability 부모 차단은 사망 고유의 명시 규칙으로 남는다. 비용·쿨다운·소유자 발동 조건·취소 선언은 기존 데이터와 기능을 유지한다.
- 에디터의 BlockAbilitiesWithTag는 개별 명시 선언이다. 자동 공통 규칙을 에셋/CDO에 써 넣지 않는다. 최종 목록은 GetAbilityBlockTags로 조회한다.

## 검증과 한계

- Editor Win64 Development: `Saved/Logs/BuildDoctor/build_2026-09-25_231328_105_36192.log`, Result Succeeded, exit 0.
- `Wx.Combat.AbilityBlocking.AssetDefaults`·`HookRules`·`Lifecycle`: 성공 3, 실패 0, 경고 0. `Saved/Tests/ExclusiveAbilityBlocking-AscHook/index.json`, `Saved/Logs/ExclusiveAbilityBlocking-AscHook.log`.
- 회귀는 클래스와 무관한 그룹·태그 계산, 명시/추가 차단 보존, 비 Wx·빈 요청자, 콤보 자기/외부 기여 구분, GE 차단, 도플갱어 면제 범위, 재발동·강공격 취소·Recovery·즉시 종료를 확인했다.
- 리다이렉트 없는 새 에디터 프로세스에서 GA 40개 CDO의 계산 결과를 대조했다. 차단 관계 1,600건과 점프 40건, 도플갱어 네 효과 참조가 통과했다. `Saved/Tests/ExclusiveAbilityAssets-AscHook.json`, `Saved/Logs/ExclusiveAbilityAssets-AscHook.log`.
- 40개 GA의 개별 명시 차단은 GA_Shared_Death의 Ability뿐이다. 앞서 확인한 저장 오버라이드 0건과 함께 GA 재저장 없이 공통 계산이 적용됨을 확인했다. ABS_Doppelganger는 앞선 이름 이관으로 이미 저장했다.
- 렌더링·몽타주를 생략한 ServerOnly 테스트다. 사람의 코드 리뷰, 실제 입력·몽타주 타이밍, 도플갱어 BT 재시도, UI, 네트워크 예측/복제는 미확인이다.

## 코드 근거

로컬 UE 5.8 GameplayAbility.cpp의 PreActivate·SetShouldBlockOtherAbilities·EndAbility와 AbilitySystemComponent_Abilities.cpp의 ApplyAbilityBlockAndCancelTags를 대조했다. ASC의 가상 함수에서 목록을 보완하고 Super에 맡기는 순정 확장 지점이며 엔진은 수정하지 않았다.

아래 SHA-256은 이 원자료 수집 시점의 작업 트리 파일 식별자다. 다른 작업의 미커밋 변경도 포함하므로 이번 작업만의 변경 목록은 아니다.

- [WxAbilityBase.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp): `bf4ccfe23eec0e6994ef1e8cb9977eace80b88e6aeb7c8fce8d3c5c80c7991e0`
- [WxAbilityBase.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h): `6fba4c3fac01a8d7c0afbd6c59ccfc356b293aba3f125b5cd0afe14307beee3c`
- [WxAbilitySystemComponent.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp): `8b850d09b73bf6885ae5655c6b3378915af1169d426b0c5907c0ccd16895ddb1`
- [WxAbilitySystemComponent.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h): `6d8644c2f72032ebb0048704e970c285c6c446e9dd646c74c5d6cc56d8eabc97`
- [WxAbility_Attack.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp): `fe100474225d8c15028fddb63831dba52acbc6cc821df367936a961a0d628e7b`
- [WxAbilityBlockingTests.cpp](../../../Source/WxEditor/Tests/WxAbilityBlockingTests.cpp): `91209b662edcf88915880e9661ad35a36acbc14fb18946bd6c16750e1be0b0c6`
