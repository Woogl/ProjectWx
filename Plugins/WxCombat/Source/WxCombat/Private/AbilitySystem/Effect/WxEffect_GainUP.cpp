// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Effect/WxEffect_GainUP.h"

#include "AbilitySystem/WxCombatAttributeSet.h"

UWxEffect_GainUP::UWxEffect_GainUP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	constexpr float UPGainOnHit = 5.f;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(UPGainOnHit));
	Modifiers.Add(Modifier);
}
