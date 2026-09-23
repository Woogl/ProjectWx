// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_MoveSpeedOverride.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_MoveSpeedOverride::UWxEffect_MoveSpeedOverride()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FSetByCallerFloat ScaleSetByCaller;
	ScaleSetByCaller.DataTag = WxGameplayTags::SetByCaller_MoveSpeedScale;

	FGameplayModifierInfo ScaleModifier;
	ScaleModifier.Attribute = UWxCombatAttributeSet::GetSPDAttribute();
	ScaleModifier.ModifierOp = EGameplayModOp::Override;
	ScaleModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScaleSetByCaller);
	Modifiers.Add(ScaleModifier);
}
