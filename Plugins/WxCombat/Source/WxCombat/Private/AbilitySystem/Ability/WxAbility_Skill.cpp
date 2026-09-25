// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// BT가 부르는 번호 태그(Ability.Skill.1 등)는 GA_가 에셋 태그와 소유 태그에 더한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Skill);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Skill);
	
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;

	bRetriggerInstancedAbility = true;
}

void UWxAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboIndex = ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;

	if (!PlayMontage(GetMontage()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}

void UWxAbility_Skill::OnComboWindowClosed()
{
	ComboIndex = INDEX_NONE;
}
