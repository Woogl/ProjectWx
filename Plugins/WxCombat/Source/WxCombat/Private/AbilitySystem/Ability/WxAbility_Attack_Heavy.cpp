// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack_Heavy.h"
#include "WxGameplayTags.h"

UWxAbility_Attack_Heavy::UWxAbility_Attack_Heavy()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack_Heavy);
	SetAssetTags(AssetTags);

	ActivationInputTag = WxGameplayTags::Input_Attack_Heavy;
	CooldownTag = WxGameplayTags::Cooldown_Attack_Heavy;
}
