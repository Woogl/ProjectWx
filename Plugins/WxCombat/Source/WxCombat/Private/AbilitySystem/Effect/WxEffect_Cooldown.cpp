// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));
}
