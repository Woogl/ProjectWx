// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_MoveSpeedOverride.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_MoveSpeedOverride::UWxEffect_MoveSpeedOverride()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FSetByCallerFloat SpeedSetByCaller;
	SpeedSetByCaller.DataTag = WxGameplayTags::SetByCaller_Magnitude;

	FGameplayModifierInfo SpeedModifier;
	SpeedModifier.Attribute = UWxCombatAttributeSet::GetMOVAttribute();
	SpeedModifier.ModifierOp = EGameplayModOp::Override;
	SpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SpeedSetByCaller);
	Modifiers.Add(SpeedModifier);
}
