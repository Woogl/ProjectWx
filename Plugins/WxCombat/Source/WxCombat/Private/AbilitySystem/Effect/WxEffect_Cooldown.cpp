// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));

	// 스태킹을 사용하지 않는다.
	// 충전(Charge) 시스템의 순차 복구(D초 간격)는 UWxAbility::ApplyCooldown에서
	// 새 GE의 Duration을 "기존 최대 잔여시간 + CooldownDuration"으로 동적 계산하여 구현한다.
}
