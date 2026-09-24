// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_SkillCutscene.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/BlockAbilityTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_SkillCutscene::UWxEffect_SkillCutscene()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_SkillCutscene);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UBlockAbilityTagsGameplayEffectComponent* BlockAbilityTagsComp = CreateDefaultSubobject<UBlockAbilityTagsGameplayEffectComponent>(TEXT("BlockAbilityTags"));
	FInheritedTagContainer BlockedAbilityTags;
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Attack);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Skill);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Ultimate);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Dodge);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Guard);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_UseItem);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Sprint);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_LockOn);
	BlockedAbilityTags.Added.AddTag(WxGameplayTags::Ability_Interact);
	BlockAbilityTagsComp->SetAndApplyBlockedAbilityTagChanges(BlockedAbilityTags);
	GEComponents.Add(BlockAbilityTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_SkillCutscene);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);
}
