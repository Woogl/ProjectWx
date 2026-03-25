// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_MPRecovery.h"
#include "AbilitySystem/WxCombatAttributeSet.h"

UWxEffect_MPRecovery::UWxEffect_MPRecovery()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	constexpr float MPRecoveryOnHit = 5.f;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(MPRecoveryOnHit));
	Modifiers.Add(Modifier);
}
