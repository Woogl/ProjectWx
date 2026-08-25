// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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

bool UWxCombatLibrary::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration)
{
	if (!Source || !Target)
	{
		return false;
	}

	AActor* SourceActor = Source->GetOwnerActor();
	const UGameplayAbility* AnimatingAbility = Source->GetAnimatingAbility();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddInstigator(SourceActor, SourceActor);
	Context.SetAbility(AnimatingAbility);
	Context.AddHitResult(HitResult);

	// 애님 노티파이는 어빌리티 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	// 적중 성립 여부는 대미지가 들어가기 전 상태로 가른다 — 이 히트로 죽은 대상은 아직 살아 있던 것으로 쳐야 마무리 일격에도 역경직이 걸린다.
	const EWxDamageResult DamageCheck = UWxExecCalc_Damage::CheckDamage(Source, Target);

	// 무적 회피는 어트리뷰트를 하나도 바꾸지 않아 적용 뒤에는 알아낼 방법이 없다. 같은 선판정을 쓰는 여기서 알린다.
	if (DamageCheck == EWxDamageResult::Evaded)
	{
		AActor* TargetActor = Target->GetOwnerActor();

		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_DodgeSuccess, EventData);
	}

	// 성립하지 않는 히트는 GE를 하나도 걸지 않는다. 대미지 GE는 값을 하나도 내지 못하면서 자기에게 얹힌 히트 큐만 발행하고, 상태이상 역시 흘려낸 히트에서는 걸리지 않아야 한다.
	if (DamageCheck != EWxDamageResult::Damaged)
	{
		return false;
	}

	FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(TEXT("GetDamageTableRow"));
	if (!DamageRow)
	{
		return false;
	}
	
	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageRow->MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, PredictionKey);
			bAppliedAny |= AppliedHandle.WasSuccessfullyApplied();
		}
	}

	// 발동만은 적용 뒤다.
	// GE 적용이 동기라 이 시점엔 피격자가 보낸 반응 이벤트(패리 등)가 도착해 있어, ApplyHitStop이 그 반응에 몽타주를 양보할 수 있다.
	if (HitStopDuration > 0.f)
	{
		if (UWxAbilitySystemComponent* SourceWxASC = Cast<UWxAbilitySystemComponent>(Source))
		{
			SourceWxASC->ApplyHitStop(HitStopDuration, AnimatingAbility);
		}
	}

	return bAppliedAny;
}

void UWxCombatLibrary::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility)
{
	if (!TargetASC || !EffectClass)
	{
		return;
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), 1.f);

	// 애님 노티파이는 어빌리티 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (PredictingAbility)
	{
		PredictionKey = PredictingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec, PredictionKey);
}

void UWxCombatLibrary::RemoveEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!TargetASC || !EffectClass)
	{
		return;
	}

	TargetASC->RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1);
}
