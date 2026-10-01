// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Skill.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// BT가 부르는 번호 태그(Ability.Action.Skill.1 등)는 GA_가 에셋 태그와 소유 태그에 더한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Action_Skill);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Skill);
	
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	bRetriggerInstancedAbility = true;
}

void UWxAbility_Skill::OnComboWindowClosed()
{
	ComboIndex = INDEX_NONE;
}
