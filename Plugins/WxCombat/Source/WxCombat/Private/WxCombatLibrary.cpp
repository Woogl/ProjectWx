// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "GenericTeamAgentInterface.h"
#include "WxGameplayTags.h"
#include "Damage/WxDamageTableRow.h"

bool UWxCombatLibrary::IsHostile(const AActor* Source, const AActor* Target)
{
	if (!Source || !Target)
	{
		return true;
	}

	const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(Source);
	return !SourceTeamAgent || SourceTeamAgent->GetTeamAttitudeTowards(*Target) == ETeamAttitude::Hostile;
}

bool UWxCombatLibrary::ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration)
{
	if (!Causer || !Target)
	{
		return false;
	}

	AActor* SourceActor = Causer;
	UAbilitySystemComponent* Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Causer);
	if (!Source)
	{
		SourceActor = Causer->GetOwner();
		Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!Source || !TargetASC)
	{
		return false;
	}

	const UGameplayAbility* AnimatingAbility = Source->GetAnimatingAbility();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddInstigator(SourceActor, Causer);
	Context.SetAbility(AnimatingAbility);
	Context.AddHitResult(HitResult);

	// 노티파이는 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	// 적용 전에 판정한다 — 이 히트로 죽는 대상에도 히트스톱이 걸려야 한다.
	const EWxDamageCheck DamageCheck = UWxExecCalc_Damage::CheckDamage(Source, TargetASC);

	if (DamageCheck == EWxDamageCheck::Evaded)
	{
		AActor* TargetActor = TargetASC->GetOwnerActor();

		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_DodgeSuccess, EventData);
	}

	// 흘려낸 히트에 GE를 걸면 히트 큐와 상태이상만 새어 나간다.
	if (DamageCheck != EWxDamageCheck::Damaged)
	{
		return false;
	}

	FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(TEXT("GetDamageTableRow"));
	if (!DamageRow)
	{
		return false;
	}
	
	bool bDamageApplied = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageRow->MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC, PredictionKey);
			if (Spec.Data->Def->IsA<UWxEffect_Damage>())
			{
				bDamageApplied = AppliedHandle.WasSuccessfullyApplied();
			}
		}
	}

	// 적용 뒤라야 동기로 도착한 반응(패리 등)에 히트스톱이 몽타주를 양보한다.
	if (HitStopDuration > 0.f)
	{
		if (UWxAbilitySystemComponent* SourceWxASC = Cast<UWxAbilitySystemComponent>(Source))
		{
			SourceWxASC->ApplyHitStop(HitStopDuration, AnimatingAbility);
		}
	}

	return bDamageApplied;
}

void UWxCombatLibrary::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility)
{
	if (!TargetASC || !EffectClass)
	{
		return;
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);

	FPredictionKey PredictionKey;
	if (PredictingAbility)
	{
		PredictionKey = PredictingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec, PredictionKey);
}
