// Copyright Woogle. All Rights Reserved.

#include "Damage/WxDamageTableRow.h"

#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Effect/WxEffect_Hit.h"

FGameplayEffectSpecHandle FWxDamageTableRow::MakeHitSpec(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const
{
	if (!SourceASC)
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectSpecHandle HitSpecHandle = SourceASC->MakeOutgoingSpec(UWxEffect_Hit::StaticClass(), 1.f, Context);
	if (HitSpecHandle.IsValid())
	{
		FGameplayEffectSpec* Spec = HitSpecHandle.Data.Get();
		Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, CoeffATK);

		// 소비 쪽은 이 태그로 치트·즉사 같은 직접 피해와 가른다.
		FGameplayTagContainer AttackTags;
		AttackTags.AddTag(WxGameplayTags::Damage_Attack);
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
		Spec->AppendDynamicAssetTags(AttackTags);
	}

	return HitSpecHandle;
}
