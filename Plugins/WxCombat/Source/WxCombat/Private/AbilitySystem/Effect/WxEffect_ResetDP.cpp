// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_ResetDP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_ResetDP::UWxEffect_ResetDP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetDPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Override;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.f));
	Modifiers.Add(Modifier);
}
