// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Kill.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_Kill::UWxEffect_Kill()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FAttributeBasedFloat AttributeBased;
	AttributeBased.Coefficient = FScalableFloat(1.f);
	AttributeBased.BackingAttribute.AttributeToCapture = UWxCombatAttributeSet::GetHPAttribute();
	AttributeBased.BackingAttribute.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	AttributeBased.BackingAttribute.bSnapshot = false;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);

	Modifiers.Add(Modifier);
}
