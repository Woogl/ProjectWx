// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_NoCooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "GameplayEffectComponents/RemoveOtherGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_NoCooldown::UWxEffect_NoCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	FGameplayEffectQuery CooldownQuery;
	CooldownQuery.EffectDefinition = UWxEffect_Cooldown::StaticClass();

	URemoveOtherGameplayEffectComponent* RemoveComp = CreateDefaultSubobject<URemoveOtherGameplayEffectComponent>(TEXT("RemoveCooldowns"));
	RemoveComp->RemoveGameplayEffectQueries.Add(CooldownQuery);
	GEComponents.Add(RemoveComp);

	UImmunityGameplayEffectComponent* ImmunityComp = CreateDefaultSubobject<UImmunityGameplayEffectComponent>(TEXT("Immunity"));
	ImmunityComp->ImmunityQueries.Add(CooldownQuery);
	GEComponents.Add(ImmunityComp);
}
