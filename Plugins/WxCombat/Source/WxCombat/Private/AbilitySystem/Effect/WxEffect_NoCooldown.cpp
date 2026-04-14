// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_NoCooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystemComponent.h"
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

	// 적용 시 기존 쿨다운 모두 제거
	URemoveOtherGameplayEffectComponent* RemoveComp = CreateDefaultSubobject<URemoveOtherGameplayEffectComponent>(TEXT("RemoveCooldowns"));
	RemoveComp->RemoveGameplayEffectQueries.Add(CooldownQuery);
	GEComponents.Add(RemoveComp);

	// 이후 새 쿨다운 적용 차단
	UImmunityGameplayEffectComponent* ImmunityComp = CreateDefaultSubobject<UImmunityGameplayEffectComponent>(TEXT("Immunity"));
	ImmunityComp->ImmunityQueries.Add(CooldownQuery);
	GEComponents.Add(ImmunityComp);
}

FActiveGameplayEffectHandle UWxEffect_NoCooldown::ApplyToASC(UAbilitySystemComponent* ASC, float Duration)
{
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StaticClass(), 1.f, Context);
	Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, Duration);
	return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}
