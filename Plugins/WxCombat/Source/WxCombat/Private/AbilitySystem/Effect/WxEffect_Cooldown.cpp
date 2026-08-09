// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxMMC_CooldownDuration.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FCustomCalculationBasedFloat DurationCalc;
	DurationCalc.CalculationClassMagnitude = UWxMMC_CooldownDuration::StaticClass();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationCalc);
}
