// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/BlockAbilityTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Invincible::UWxEffect_Invincible()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Damage GE만 막는다. 이 차단 통지를 Dodge가 극한 회피로 받는다.
	UImmunityGameplayEffectComponent* ImmunityComp = CreateDefaultSubobject<UImmunityGameplayEffectComponent>(TEXT("Immunity"));
	FGameplayEffectQuery DamageQuery;
	DamageQuery.EffectDefinition = UWxEffect_Damage::StaticClass();
	ImmunityComp->ImmunityQueries.Add(DamageQuery);
	GEComponents.Add(ImmunityComp);

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_Invincible);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	// 무적 대상에겐 Event.Hit이 오지 않지만, 공격자 쪽으로 가는 Event.Hit.Parry는 여기서 막힌다.
	UBlockAbilityTagsGameplayEffectComponent* BlockAbilityTagsComp = CreateDefaultSubobject<UBlockAbilityTagsGameplayEffectComponent>(TEXT("BlockAbilityTags"));
	FInheritedTagContainer BlockedAbilityTags;
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_HitReact);
	BlockAbilityTagsComp->SetAndApplyBlockedAbilityTagChanges(BlockedAbilityTags);
	GEComponents.Add(BlockAbilityTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_Invincible);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);
}
