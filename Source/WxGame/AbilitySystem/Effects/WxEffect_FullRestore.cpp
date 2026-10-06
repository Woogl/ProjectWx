// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effects/WxEffect_FullRestore.h"
#include "AbilitySystem/Attributes/WxCombatAttributeSet.h"

UWxEffect_FullRestore::UWxEffect_FullRestore()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	const TPair<FGameplayAttribute, FGameplayAttribute> RestoredToMax[] = {
		{ UWxCombatAttributeSet::GetHPAttribute(), UWxCombatAttributeSet::GetMaxHPAttribute() },
		{ UWxCombatAttributeSet::GetMPAttribute(), UWxCombatAttributeSet::GetMaxMPAttribute() },
	};
	for (const TPair<FGameplayAttribute, FGameplayAttribute>& Pair : RestoredToMax)
	{
		FAttributeBasedFloat MaxBased;
		MaxBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
			Pair.Value,
			EGameplayEffectAttributeCaptureSource::Target,
			false);
		MaxBased.Coefficient = 1.f;
		MaxBased.PreMultiplyAdditiveValue = 0.f;
		MaxBased.PostMultiplyAdditiveValue = 0.f;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Pair.Key;
		Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxBased);
		Modifiers.Add(Modifier);
	}
}
