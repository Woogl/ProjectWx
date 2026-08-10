// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Damage/WxDamageInfo.h"
#include "WxGameplayTags.h"

bool UWxCombatLibrary::ApplyDamage(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, const FWxDamageInfo& DamageInfo, const FHitResult& HitResult, float HitStopDuration)
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

	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			if (HitStopDuration > 0.f)
			{
				Spec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_HitStop, HitStopDuration);
			}

			const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, PredictionKey);
			bAppliedAny |= AppliedHandle.WasSuccessfullyApplied();
		}
	}

	return bAppliedAny;
}
