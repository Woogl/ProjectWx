// Copyright Woogle. All Rights Reserved.

#include "Damage/WxDamageInfo.h"
#include "Damage/WxDamageTableRow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "WxGameplayTags.h"

FWxDamageInfo::FWxDamageInfo()
{
	HitReactTag = WxGameplayTags::Event_HitReact_Normal;
}

FWxDamageInfo FWxDamageInfo::FromDataRow(const FDataTableRowHandle& RowHandle)
{
	FWxDamageInfo DamageInfo;
	if (const FWxDamageTableRow* Row = RowHandle.GetRow<FWxDamageTableRow>(RowHandle.ToDebugString()))
	{
		DamageInfo.CoeffATK = Row->CoeffATK;
		DamageInfo.RecoverMP = Row->RecoverMP;
		DamageInfo.RecoverUP = Row->RecoverUP;
		DamageInfo.HitReactTag = Row->HitReactTag;
		DamageInfo.bCanCritical = Row->bCanCritical;
		DamageInfo.bUnblockable = Row->bUnblockable;
		DamageInfo.bParryHitReact = Row->bParryHitReact;
		DamageInfo.AdditionalEffects = Row->AdditionalEffects;
	}
	return DamageInfo;
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
		if (bCanCritical)
		{
			AttackTags.AddTag(WxGameplayTags::Damage_CanCritical);
		}
		if (bUnblockable)
		{
			AttackTags.AddTag(WxGameplayTags::Damage_Unblockable);
		}
		if (bParryHitReact)
		{
			AttackTags.AddTag(WxGameplayTags::Damage_ParryHitReact);
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
