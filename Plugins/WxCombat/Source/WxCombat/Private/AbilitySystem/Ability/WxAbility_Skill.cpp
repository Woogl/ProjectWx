// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// 슬롯마다 다른 애셋 태그(Ability.Skill.1~4)와 입력 액션은 BP 서브클래스가 지정한다.
	// 슬롯 태그는 BP 소관이라 코드가 알 수 없으므로 부모 태그로 활성 표식을 보장한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Skill);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 스킬은 재생 중 다른 GA로 캔슬되지 않는다. (PC규격서 §5.2)
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive_Blocking;

	bRetriggerInstancedAbility = true;
}

bool UWxAbility_Skill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	// 콤보 진행. 점유자가 자기 자신이고 엔진 재발동이 풀었다 다시 쥐므로 배타 판정을 건너뛴다.
	if (Spec && Spec->IsActive())
	{
		if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::State_ComboWindow))
		{
			return false;
		}
		if (ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags))
		{
			return false;
		}
		return CheckCooldown(Handle, ActorInfo, OptionalRelevantTags) && CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWxAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 터미널 단에서는 첫 단으로 되감긴다.
	ComboIndex = ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;

	UAnimMontage* ComboMontage = ComboMontages.IsValidIndex(ComboIndex) ? ComboMontages[ComboIndex].Get() : nullptr;
	if (!PlayMontage(ComboMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 캔슬 종료만 되돌린다 — 콤보 재발동도 이 종료를 지나가는데, 그쪽은 bWasCancelled=false라 진행 단이 보존된다.
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	// 캔슬이 아니라 EndAbility가 되돌려주지 않는다.
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}
