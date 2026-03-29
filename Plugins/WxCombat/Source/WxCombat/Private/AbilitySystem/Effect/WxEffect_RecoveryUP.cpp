// Copyright Woogle. All Rights Reserved.


#include "AbilitySystem/Effect/WxEffect_GainUP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_RecoveryUP::UWxEffect_RecoveryUP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Recovery;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
