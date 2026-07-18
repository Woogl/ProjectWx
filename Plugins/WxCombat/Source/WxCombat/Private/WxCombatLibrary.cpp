// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystemComponent.h"
#include "WxDamageInfo.h"
#include "WxGameplayTags.h"

bool UWxCombatLibrary::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration)
{
	if (!Source || !Target)
	{
		return false;
	}

	AActor* SourceActor = Source->GetOwnerActor();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddInstigator(SourceActor, SourceActor);
	Context.SetAbility(Source->GetAnimatingAbility());
	Context.AddHitResult(HitResult);

	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
			bAppliedAny = true;
		}
	}

	if (bAppliedAny && HitStopDuration > 0.f)
	{
		FGameplayCueParameters HitStopParams;
		HitStopParams.RawMagnitude = HitStopDuration;
		Source->ExecuteGameplayCue(WxGameplayTags::GameplayCue_HitStop, HitStopParams);
	}

	return bAppliedAny;
}

bool UWxCombatLibrary::ApplyRawDamage(UAbilitySystemComponent* Target, float DamageAmount, FGameplayTag HitReaction)
{
	if (!Target || DamageAmount <= 0.f)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = Target->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = Target->MakeOutgoingSpec(UWxEffect_Damage::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	// SetByCaller.RawDamage 가 양수이면 ExecCalc 가 ATK·DEF 우회 모드로 동작한다.
	Spec->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_RawDamage, DamageAmount);

	// HitReact 태그는 DynamicAssetTags 로 실어야 ExecCalc 의 HitReact 분기가 인식한다.
	if (HitReaction.IsValid())
	{
		const FGameplayTagContainer AssetTag = HitReaction.GetSingleTagContainer();
		Spec->AppendDynamicAssetTags(AssetTag);
	}

	Target->ApplyGameplayEffectSpecToSelf(*Spec);

	return true;
}
