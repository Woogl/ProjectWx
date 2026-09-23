// Copyright Woogle. All Rights Reserved.

#include "Damage/WxDamageTableRow.h"

#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "Damage/WxDamageEffectContext.h"

FGameplayEffectSpecHandle FWxDamageTableRow::MakeDamageSpec(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context, float DamageLevel) const
{
	if (!SourceASC)
	{
		return FGameplayEffectSpecHandle();
	}

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

	const FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(UWxEffect_Damage::StaticClass(), DamageLevel, Context);
	if (DamageSpecHandle.IsValid())
	{
		FGameplayEffectSpec* Spec = DamageSpecHandle.Data.Get();
		Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, CoeffATK);
		Spec->AppendDynamicAssetTags(AttackTags);
	}

	if (FWxDamageEffectContext* DamageContext = FWxDamageEffectContext::Get(Context))
	{
		DamageContext->AdditionalEffects = AdditionalEffects;
	}

	return DamageSpecHandle;
}
