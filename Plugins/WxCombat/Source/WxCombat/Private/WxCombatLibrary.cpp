// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "Damage/WxHitEffectContext.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "GenericTeamAgentInterface.h"
#include "Damage/WxDamageTableRow.h"

bool UWxCombatLibrary::IsHostile(const AActor* Source, const AActor* Target)
{
	const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(Source);
	if (!SourceTeamAgent)
	{
		return false;
	}
	
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Target);
	if (!TargetTeamAgent)
	{
		return false;
	}
	
	return SourceTeamAgent->GetTeamAttitudeTowards(*Target) == ETeamAttitude::Hostile;
}

bool UWxCombatLibrary::ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult)
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

	FGameplayEffectContextHandle Context(new FWxHitEffectContext(*Source->MakeEffectContext().Get(), DamageTableRow));
	Context.AddInstigator(SourceActor, Causer);
	Context.SetAbility(AnimatingAbility);
	Context.AddHitResult(HitResult);

	// 노티파이는 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	const FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(ANSI_TO_TCHAR(__FUNCTION__));
	if (!DamageRow)
	{
		return false;
	}

	const FGameplayEffectSpecHandle HitSpec = DamageRow->MakeHitSpec(Source, Context);
	if (!HitSpec.IsValid())
	{
		return false;
	}

	Source->ApplyGameplayEffectSpecToTarget(*HitSpec.Data.Get(), TargetASC, PredictionKey);
	// Wrapper 접수와 자식 피해 적용은 다르다. 회피와 자식 거부에서는 히트스톱을 켜지 않는다.
	return FWxHitEffectContext::Get(Context)->bDamageApplied;
}

void UWxCombatLibrary::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility)
{
	if (!TargetASC || !EffectClass)
	{
		return;
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	// 어빌리티 없이 걸리는 GE 도 GetAbilityLevel 의 기본 반환과 같은 레벨 1 로 만든다.
	const float Level = PredictingAbility ? PredictingAbility->GetAbilityLevel() : 1.f;
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), Level);

	FPredictionKey PredictionKey;
	if (PredictingAbility)
	{
		PredictionKey = PredictingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec, PredictionKey);
}
