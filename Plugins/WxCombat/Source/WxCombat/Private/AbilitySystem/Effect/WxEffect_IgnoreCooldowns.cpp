// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_IgnoreCooldowns.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "GameplayEffectComponents/RemoveOtherGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_IgnoreCooldowns::UWxEffect_IgnoreCooldowns()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 공용 쿨다운 GE가 부모 태그를 부여하므로 부모 태그로 한 번에 잡는다.
	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(WxGameplayTags::Cooldown));

	URemoveOtherGameplayEffectComponent* RemoveComp = CreateDefaultSubobject<URemoveOtherGameplayEffectComponent>(TEXT("RemoveCooldowns"));
	RemoveComp->RemoveGameplayEffectQueries.Add(CooldownQuery);
	GEComponents.Add(RemoveComp);

	UImmunityGameplayEffectComponent* ImmunityComp = CreateDefaultSubobject<UImmunityGameplayEffectComponent>(TEXT("Immunity"));
	ImmunityComp->ImmunityQueries.Add(CooldownQuery);
	GEComponents.Add(ImmunityComp);
}
