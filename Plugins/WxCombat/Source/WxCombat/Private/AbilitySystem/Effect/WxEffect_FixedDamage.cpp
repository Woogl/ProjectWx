// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_FixedDamage.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_FixedDamage::UWxEffect_FixedDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageSetByCaller;
	DamageSetByCaller.DataTag = WxGameplayTags::SetByCaller_FixedDamage;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UWxCombatAttributeSet::GetIncomingDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageSetByCaller);
	Modifiers.Add(DamageModifier);
}
