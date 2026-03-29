// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_GainMP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_RecoveryMP::UWxEffect_RecoveryMP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Recovery;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
