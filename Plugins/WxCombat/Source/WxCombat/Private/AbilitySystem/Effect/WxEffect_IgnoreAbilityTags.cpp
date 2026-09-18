// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_IgnoreAbilityTags.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace WxCombatGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Effect_IgnoreAbilityTags, "Effect.IgnoreAbilityTags");
}

UWxEffect_IgnoreAbilityTags::UWxEffect_IgnoreAbilityTags()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxCombatGameplayTags::Effect_IgnoreAbilityTags);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxCombatGameplayTags::Effect_IgnoreAbilityTags);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);
}
