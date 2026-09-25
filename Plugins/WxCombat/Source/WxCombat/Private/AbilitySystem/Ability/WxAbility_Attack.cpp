// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	SetAttackTag(WxGameplayTags::Ability_Attack);

	ActivationGroup = EWxAbilityActivationGroup::Exclusive;

	bRetriggerInstancedAbility = true;
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Attack::HandleMontageCompleted()
{
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}

void UWxAbility_Attack::OnComboWindowClosed()
{
	ComboIndex = INDEX_NONE;
}

void UWxAbility_Attack::SetAttackTag(const FGameplayTag& AttackTag)
{
	SetAssetTags(FGameplayTagContainer(AttackTag));

	ActivationOwnedTags.Reset();
	ActivationOwnedTags.AddTag(AttackTag);
}

UWxAbility_Attack_Light::UWxAbility_Attack_Light()
{
	SetAttackTag(WxGameplayTags::Ability_Attack_Light);

	ActivationBlockedTags.AddTag(WxGameplayTags::Movement_InAir);
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Dodge);
}

UWxAbility_Attack_Heavy::UWxAbility_Attack_Heavy()
{
	SetAttackTag(WxGameplayTags::Ability_Attack_Heavy);

	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack_Light);

	ActivationBlockedTags.AddTag(WxGameplayTags::Movement_InAir);
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Dodge);
}

UWxAbility_Attack_Air::UWxAbility_Attack_Air()
{
	SetAttackTag(WxGameplayTags::Ability_Attack_Air);

	ActivationRequiredTags.AddTag(WxGameplayTags::Movement_InAir);
}

UWxAbility_Attack_DodgeCounter::UWxAbility_Attack_DodgeCounter()
{
	SetAttackTag(WxGameplayTags::Ability_Attack_DodgeCounter);

	ActivationRequiredTags.AddTag(WxGameplayTags::Ability_Dodge);
	ActivationBlockedTags.AddTag(WxGameplayTags::Movement_InAir);
}
