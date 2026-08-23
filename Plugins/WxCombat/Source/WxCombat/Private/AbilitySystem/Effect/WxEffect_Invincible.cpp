// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Invincible::UWxEffect_Invincible()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_Invincible);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_Invincible);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);
}
