// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Effect/WxEffect_CostUP.h"

#include "WxGameplayTags.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_CostUP::UWxEffect_CostUP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Cost;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
