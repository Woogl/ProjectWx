// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_NoCooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_NoCooldown::UWxEffect_NoCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	// WxEffect_Cooldown 적용 차단
	UImmunityGameplayEffectComponent* ImmunityComp = CreateDefaultSubobject<UImmunityGameplayEffectComponent>(TEXT("Immunity"));
	FGameplayEffectQuery Query;
	Query.EffectDefinition = UWxEffect_Cooldown::StaticClass();
	ImmunityComp->ImmunityQueries.Add(Query);
	GEComponents.Add(ImmunityComp);
}

FActiveGameplayEffectHandle UWxEffect_NoCooldown::ApplyToASC(UAbilitySystemComponent* ASC, float Duration)
{
	// 기존 쿨다운 모두 제거
	FGameplayEffectQuery CooldownQuery;
	CooldownQuery.EffectDefinition = UWxEffect_Cooldown::StaticClass();
	ASC->RemoveActiveEffects(CooldownQuery);

	// NoCooldown 적용
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StaticClass(), 1.f, Context);
	Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, Duration);
	return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}
