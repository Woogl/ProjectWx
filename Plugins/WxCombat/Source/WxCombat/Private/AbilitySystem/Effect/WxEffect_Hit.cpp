// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Hit.h"
#include "AbilitySystem/Effect/WxEffectComponent_Hit.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Hit::UWxEffect_Hit()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	UTargetTagRequirementsGameplayEffectComponent* Requirements = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);
	GEComponents.Add(Requirements);
	GEComponents.Add(CreateDefaultSubobject<UWxEffectComponent_Hit>(TEXT("Hit")));
}
