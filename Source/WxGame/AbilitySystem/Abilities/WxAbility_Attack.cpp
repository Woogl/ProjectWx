// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Attack.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	bRetriggerInstancedAbility = true;
}

void UWxAbility_Attack::OnComboWindowClosed()
{
	ComboIndex = INDEX_NONE;
}

UWxAbility_Attack_Light::UWxAbility_Attack_Light()
{
	SetAssetTags(FGameplayTagContainer(WxGameplayTags::Ability_Action_Attack_Light));
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Attack_Light);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	ActivationBlockedTags.AddTag(WxGameplayTags::Movement_InAir);
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Action_Dodge);
}

UWxAbility_Attack_Heavy::UWxAbility_Attack_Heavy()
{
	SetAssetTags(FGameplayTagContainer(WxGameplayTags::Ability_Action_Attack_Heavy));
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Attack_Heavy);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action_Attack_Light);

	ActivationBlockedTags.AddTag(WxGameplayTags::Movement_InAir);
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Action_Dodge);
}

UWxAbility_Attack_Air::UWxAbility_Attack_Air()
{
	SetAssetTags(FGameplayTagContainer(WxGameplayTags::Ability_Action_Attack_Air));
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Attack_Air);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	ActivationRequiredTags.AddTag(WxGameplayTags::Movement_InAir);
}

UWxAbility_Attack_DodgeCounter::UWxAbility_Attack_DodgeCounter()
{
	SetAssetTags(FGameplayTagContainer(WxGameplayTags::Ability_Action_Attack_DodgeCounter));
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Attack_DodgeCounter);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	ActivationRequiredTags.AddTag(WxGameplayTags::Ability_Action_Dodge);
	ActivationBlockedTags.AddTag(WxGameplayTags::Movement_InAir);
}
