// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxGameplayAbility.h"
#include "AbilitySystemComponent.h"

UWxGameplayAbility::UWxGameplayAbility()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UWxGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent.Get()->TryActivateAbility(Spec.Handle, false);
	}
}
