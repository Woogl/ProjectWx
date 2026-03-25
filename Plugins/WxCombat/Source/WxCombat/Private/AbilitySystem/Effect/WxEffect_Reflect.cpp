// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Reflect.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_Reflect::UWxEffect_Reflect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_ReflectDP;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetDPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
