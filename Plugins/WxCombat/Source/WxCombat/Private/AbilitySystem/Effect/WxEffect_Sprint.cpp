// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Effect/WxEffect_Sprint.h"

#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_Sprint::UWxEffect_Sprint()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetSPDAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(SprintSpeedBonus));
	Modifiers.Add(Modifier);
}
