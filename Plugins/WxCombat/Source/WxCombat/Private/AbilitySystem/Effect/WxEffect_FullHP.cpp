// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_FullHP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_FullHP::UWxEffect_FullHP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FAttributeBasedFloat MaxHPBased;
	MaxHPBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxHPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	MaxHPBased.Coefficient = 1.f;
	MaxHPBased.PreMultiplyAdditiveValue = 0.f;
	MaxHPBased.PostMultiplyAdditiveValue = 0.f;

	FGameplayModifierInfo HPModifier;
	HPModifier.Attribute = UWxCombatAttributeSet::GetHPAttribute();
	HPModifier.ModifierOp = EGameplayModOp::Override;
	HPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxHPBased);
	Modifiers.Add(HPModifier);
}
