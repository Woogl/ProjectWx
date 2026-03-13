// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffectComp_SendHitReactEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WxGameplayTags.h"

void UWxEffectComp_SendHitReactEvent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	if (!TargetASC)
	{
		return;
	}

	AActor* TargetActor = TargetASC->GetOwnerActor();
	if (!TargetActor)
	{
		return;
	}

	const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();

	FGameplayEventData EventData;
	EventData.Instigator = Context.GetOriginalInstigator();
	EventData.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_HitReact, EventData);
}
