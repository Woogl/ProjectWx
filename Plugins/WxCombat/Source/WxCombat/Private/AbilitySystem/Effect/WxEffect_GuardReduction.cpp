// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_GuardReduction.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffectComponent_Table.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_GuardReduction::UWxEffect_GuardReduction()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(WxGameplayTags::Effect_GuardReduction);
	TargetTagsComp->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTagsComp);

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(WxGameplayTags::Effect_GuardReduction);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
	GEComponents.Add(AssetTagsComp);

	FCustomCalculationBasedFloat RowMagnitude;
	RowMagnitude.CalculationClassMagnitude = UWxMMC_EffectMagnitude::StaticClass();

	FGameplayModifierInfo ReductionModifier;
	ReductionModifier.Attribute = UWxCombatAttributeSet::GetGuardReductionScaleAttribute();
	// 기본값 0에서 올려야 하므로 배율이 아니라 가산이다.
	ReductionModifier.ModifierOp = EGameplayModOp::Additive;
	ReductionModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RowMagnitude);
	Modifiers.Add(ReductionModifier);
}
