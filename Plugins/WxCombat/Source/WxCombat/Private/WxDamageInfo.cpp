// Copyright Woogle. All Rights Reserved.

#include "WxDamageInfo.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "WxGameplayTags.h"

FWxDamageInfo::FWxDamageInfo()
{
	HitReactTag = WxGameplayTags::Event_HitReact_Normal;
}

FGameplayEffectSpecHandle FWxDamageInfo::MakeDamageSpec(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const
{
	if (!SourceASC)
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UWxEffect_Damage::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return SpecHandle;
	}

	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, CoeffATK);
	Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, RecoverMP);
	Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, RecoverUP);

	FGameplayTagContainer AttackTags;
	if (HitReactTag.IsValid())
	{
		AttackTags.AddTag(HitReactTag);
	}
	if (bUnblockable)
	{
		AttackTags.AddTag(WxGameplayTags::Damage_Unblockable);
	}
	if (!AttackTags.IsEmpty())
	{
		Spec->AppendDynamicAssetTags(AttackTags);
	}

	return SpecHandle;
}
