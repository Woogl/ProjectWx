// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemGlobals.h"
#include "Damage/WxCombatEffectContext.h"

FGameplayEffectContext* UWxAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FWxCombatEffectContext();
}
