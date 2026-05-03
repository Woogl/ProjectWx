// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "WxDamageInfo.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Effect/WxEffect_FixedDamage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GenericTeamAgentInterface.h"

bool UWxCombatLibrary::ApplyDamage(AActor* Instigator, AActor* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, UObject* SourceObject, float HitStopDuration)
{
	if (!Instigator || !Target || Instigator == Target)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!SourceASC || !TargetASC)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(SourceObject ? SourceObject : Instigator);
	Context.AddInstigator(Instigator, Instigator);
	Context.SetAbility(SourceASC->GetAnimatingAbility());
	Context.AddHitResult(HitResult);

	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(SourceASC, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			bAppliedAny = true;
		}
	}

	if (bAppliedAny && HitStopDuration > 0.f)
	{
		FGameplayCueParameters HitStopParams;
		HitStopParams.RawMagnitude = HitStopDuration;
		SourceASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_HitStop, HitStopParams);
	}

	return bAppliedAny;
}

bool UWxCombatLibrary::ApplyFixedDamage(AActor* Target, float Damage, AActor* Instigator, UObject* SourceObject)
{
	if (!Target || Damage <= 0.f)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC)
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
	{
		return false;
	}

	AActor* EffectiveInstigator = Instigator ? Instigator : Target;
	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(SourceObject ? SourceObject : EffectiveInstigator);
	Context.AddInstigator(EffectiveInstigator, EffectiveInstigator);

	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(UWxEffect_FixedDamage::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_FixedDamage, Damage);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}

bool UWxCombatLibrary::IsHostile(const AActor* Source, const AActor* Target)
{
	if (!Source || !Target)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeam = Cast<IGenericTeamAgentInterface>(Source);
	if (!SourceTeam)
	{
		return true;
	}

	return SourceTeam->GetTeamAttitudeTowards(*Target) == ETeamAttitude::Hostile;
}
