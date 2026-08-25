// Copyright Woogle. All Rights Reserved.

#include "Damage/WxDamageTableRow.h"

#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"

TArray<FGameplayEffectSpecHandle> FWxDamageTableRow::MakeSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const
{
	TArray<FGameplayEffectSpecHandle> Specs;
	if (!SourceASC)
	{
		return Specs;
	}

	const FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(UWxEffect_Damage::StaticClass(), 1.f, Context);
	if (DamageSpecHandle.IsValid())
	{
		FGameplayEffectSpec* Spec = DamageSpecHandle.Data.Get();
		Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, CoeffATK);

		FGameplayTagContainer AttackTags;
		if (HitReactTag.IsValid())
		{
			AttackTags.AddTag(HitReactTag);
		}
		if (bCanCritical)
		{
			AttackTags.AddTag(WxGameplayTags::Damage_CanCritical);
		}
		if (bCanGuard)
		{
			AttackTags.AddTag(WxGameplayTags::Damage_CanGuard);
		}
		if (bCanParry)
		{
			AttackTags.AddTag(WxGameplayTags::Damage_CanParry);
		}
		if (!AttackTags.IsEmpty())
		{
			Spec->AppendDynamicAssetTags(AttackTags);
		}

		Specs.Add(DamageSpecHandle);
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : AdditionalEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		const FGameplayEffectSpecHandle AdditionalSpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (AdditionalSpecHandle.IsValid())
		{
			Specs.Add(AdditionalSpecHandle);
		}
	}

	return Specs;
}
