// Copyright Woogle. All Rights Reserved.

#include "WxDamageInfo.h"
#include "WxDamageTableRow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "WxGameplayTags.h"

FWxDamageInfo::FWxDamageInfo()
{
	HitReactTag = WxGameplayTags::Event_HitReact_Normal;
}

void FWxDamageInfo::ApplyTableRow(const FWxDamageTableRow& Row)
{
	CoeffATK = Row.CoeffATK;
	RecoverMP = Row.RecoverMP;
	RecoverUP = Row.RecoverUP;
	HitReactTag = Row.HitReactTag;
	bUnblockable = Row.bUnblockable;
	AdditionalEffects = Row.AdditionalEffects;
}

TArray<FGameplayEffectSpecHandle> FWxDamageInfo::MakeSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& Context) const
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
