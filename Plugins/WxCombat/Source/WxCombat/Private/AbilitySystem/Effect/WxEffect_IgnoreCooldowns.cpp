// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_IgnoreCooldowns.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "GameplayEffectComponents/RemoveOtherGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_IgnoreCooldowns::UWxEffect_IgnoreCooldowns()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// EffectDefinition 쿼리는 CDO 정확 일치라 어빌리티별 파생 쿨다운 GE를 놓친다. 부여 태그의 부모로 한 번에 잡는다.
	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(WxGameplayTags::Cooldown));

	URemoveOtherGameplayEffectComponent* RemoveComp = CreateDefaultSubobject<URemoveOtherGameplayEffectComponent>(TEXT("RemoveCooldowns"));
	RemoveComp->RemoveGameplayEffectQueries.Add(CooldownQuery);
	GEComponents.Add(RemoveComp);

	UImmunityGameplayEffectComponent* ImmunityComp = CreateDefaultSubobject<UImmunityGameplayEffectComponent>(TEXT("Immunity"));
	ImmunityComp->ImmunityQueries.Add(CooldownQuery);
	GEComponents.Add(ImmunityComp);
}
