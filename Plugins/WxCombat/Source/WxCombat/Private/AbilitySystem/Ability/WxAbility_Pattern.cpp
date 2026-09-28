// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Pattern.h"
#include "WxGameplayTags.h"

UWxAbility_Pattern::UWxAbility_Pattern()
{
	// BT가 부르는 번호 태그(Ability.Action.Pattern.1 등)는 GA_가 에셋 태그와 소유 태그에 더한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Action_Pattern);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Pattern);
	
	// 피격은 공격·스킬만 끊으므로, 패턴은 그로기·사망에만 끊긴다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);
}

void UWxAbility_Pattern::HandleMontageBlendOut()
{
	if (!ComboMontages.IsValidIndex(ComboIndex + 1))
	{
		return;
	}

	ComboIndex = ComboIndex + 1;
	if (!PlayMontage(GetMontage()))
	{
		ComboIndex = INDEX_NONE;
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}
