// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_ResetGP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_ResetGP::UWxEffect_ResetGP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetGPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Override;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.f));
	Modifiers.Add(Modifier);
}
