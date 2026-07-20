// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxMMC_CooldownDuration.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Duration = 소스 어빌리티 AbilityDataRow의 CooldownTime + 직렬 회복분. MMC가 계산 시점에 조회한다.
	FCustomCalculationBasedFloat DurationCalc;
	DurationCalc.CalculationClassMagnitude = UWxMMC_CooldownDuration::StaticClass();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationCalc);
}
