// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Damage/WxDamageInfo.h"

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

	// 적중 성립 여부는 대미지가 들어가기 전 상태로 가른다 — 이 히트로 죽은 대상은 아직 살아 있던 것으로 쳐야 마무리 일격에도 역경직이 걸린다.
	const bool bHitLands = HitStopDuration > 0.f && UWxExecCalc_Damage::CheckDamage(Source, Target) == EWxDamageResult::Damaged;

	bool bAppliedAny = false;
	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(Source, Context);
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
	if (bHitLands)
	{
		if (UWxAbilitySystemComponent* SourceWxASC = Cast<UWxAbilitySystemComponent>(Source))
		{
			SourceWxASC->ApplyHitStop(HitStopDuration, AnimatingAbility);
		}
	}

	return bAppliedAny;
}
