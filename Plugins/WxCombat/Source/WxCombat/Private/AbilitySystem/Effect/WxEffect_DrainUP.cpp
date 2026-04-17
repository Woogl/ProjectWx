// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_DrainUP.h"
#include "AbilitySystem/Effect/WxMMC_LinearDrain.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_DrainUP::UWxEffect_DrainUP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Period = FScalableFloat(DrainPeriod);
	bExecutePeriodicEffectOnApplication = true;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UWxMMC_LinearDrain::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(1.0f);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);
	Modifiers.Add(Modifier);
}
