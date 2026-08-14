// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Guard.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Guard::UWxEffect_Guard()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_Guard);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_Guard);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);
}

float UWxEffect_Guard::GetDamageReductionRate()
{
	return GetDefault<UWxEffect_Guard>()->DamageReductionRate;
}
