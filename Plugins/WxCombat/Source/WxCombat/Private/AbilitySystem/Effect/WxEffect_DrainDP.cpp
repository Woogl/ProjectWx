// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_DrainDP.h"
#include "AbilitySystem/Effect/WxMMC_LinearDrain.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_DrainDP::UWxEffect_DrainDP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	Period = FScalableFloat(DrainPeriod);
	bExecutePeriodicEffectOnApplication = true;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UWxMMC_LinearDrain::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(1.0f);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetDPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);
	Modifiers.Add(Modifier);
}
