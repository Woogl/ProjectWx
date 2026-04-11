// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_RecoverResource::UWxEffect_RecoverResource()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat UPSetByCaller;
	UPSetByCaller.DataTag = WxGameplayTags::SetByCaller_Recovery_UP;

	FGameplayModifierInfo UPModifier;
	UPModifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	UPModifier.ModifierOp = EGameplayModOp::Additive;
	UPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(UPSetByCaller);
	Modifiers.Add(UPModifier);

	FSetByCallerFloat MPSetByCaller;
	MPSetByCaller.DataTag = WxGameplayTags::SetByCaller_Recovery_MP;

	FGameplayModifierInfo MPModifier;
	MPModifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
	MPModifier.ModifierOp = EGameplayModOp::Additive;
	MPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MPSetByCaller);
	Modifiers.Add(MPModifier);
}
