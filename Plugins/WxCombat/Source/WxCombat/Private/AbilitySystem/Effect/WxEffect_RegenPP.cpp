// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_RegenPP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"

UWxEffect_RegenPP::UWxEffect_RegenPP()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Period = FScalableFloat(RegenPeriod);
	bExecutePeriodicEffectOnApplication = true;

	// 틱당 회복량 = MaxPP * (RegenPeriod / FullRegenDuration) = MaxPP / 120
	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxPPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	AttributeBased.Coefficient = FScalableFloat(RegenPeriod / FullRegenDuration);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetPPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
	Modifiers.Add(Modifier);
}
