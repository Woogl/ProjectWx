// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack_Light.h"
#include "WxGameplayTags.h"

UWxAbility_Attack_Light::UWxAbility_Attack_Light()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack_Light);
	SetAssetTags(AssetTags);

	ActivationInputTag = WxGameplayTags::Input_Attack_Light;
	CooldownTag = WxGameplayTags::Cooldown_Attack_Light;
}
