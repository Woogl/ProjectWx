// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_GainMP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"

UWxEffect_GainMP::UWxEffect_GainMP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	constexpr float MPGainOnHit = 5.f;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(MPGainOnHit));
	Modifiers.Add(Modifier);
}
