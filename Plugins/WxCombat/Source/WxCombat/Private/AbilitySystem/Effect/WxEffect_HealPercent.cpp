// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_HealPercent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_HealPercent::UWxEffect_HealPercent()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FAttributeBasedFloat MaxHPBased;
	MaxHPBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxHPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	MaxHPBased.Coefficient = 0.4f;
	MaxHPBased.PreMultiplyAdditiveValue = 0.f;
	MaxHPBased.PostMultiplyAdditiveValue = 0.f;

	FGameplayModifierInfo HPModifier;
	HPModifier.Attribute = UWxCombatAttributeSet::GetHPAttribute();
	HPModifier.ModifierOp = EGameplayModOp::Additive;
	HPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxHPBased);
	Modifiers.Add(HPModifier);
}
