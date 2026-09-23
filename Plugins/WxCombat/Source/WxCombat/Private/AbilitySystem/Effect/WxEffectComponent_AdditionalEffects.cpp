// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComponent_AdditionalEffects.h"
#include "AbilitySystemComponent.h"
#include "Damage/WxDamageEffectContext.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

void UWxEffectComponent_AdditionalEffects::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
	UAbilitySystemComponent* Target = ActiveGEContainer.Owner;
	UAbilitySystemComponent* Source = GESpec.GetContext().GetInstigatorAbilitySystemComponent();
	FWxDamageEffectContext* Context = FWxDamageEffectContext::Get(GESpec.GetContext());
	if (!Target || !Source || !Context)
	{
		return;
	}

	if (GESpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_PerfectGuarded))
	{
		return;
	}

	// 반응이 끝난 뒤의 출처 상태로 Spec을 만든다.
	for (const TSubclassOf<UGameplayEffect>& EffectClass : Context->AdditionalEffects)
	{
		if (EffectClass)
		{
			const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), GESpec.GetContext());
			if (Spec.IsValid())
			{
				Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target, PredictionKey);
			}
		}
	}
}
