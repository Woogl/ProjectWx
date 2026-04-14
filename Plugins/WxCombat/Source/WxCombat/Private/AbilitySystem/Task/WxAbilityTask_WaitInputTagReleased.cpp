// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_WaitInputTagReleased.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

UWxAbilityTask_WaitInputTagReleased* UWxAbilityTask_WaitInputTagReleased::CreateTask(UGameplayAbility* OwningAbility, FGameplayTag InInputTag)
{
	UWxAbilityTask_WaitInputTagReleased* Task = NewAbilityTask<UWxAbilityTask_WaitInputTagReleased>(OwningAbility);
	Task->InputTag = InInputTag;
	return Task;
}

void UWxAbilityTask_WaitInputTagReleased::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !Ability || !IsLocallyControlled())
	{
		EndTask();
		return;
	}

	DelegateHandle = ASC->AbilityReplicatedEventDelegate(
		EAbilityGenericReplicatedEvent::InputReleased,
		GetAbilitySpecHandle(),
		GetActivationPredictionKey()
	).AddUObject(this, &UWxAbilityTask_WaitInputTagReleased::HandleInputReleased);
}

void UWxAbilityTask_WaitInputTagReleased::OnDestroy(bool AbilityEnded)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->AbilityReplicatedEventDelegate(
			EAbilityGenericReplicatedEvent::InputReleased,
			GetAbilitySpecHandle(),
			GetActivationPredictionKey()
		).Remove(DelegateHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UWxAbilityTask_WaitInputTagReleased::HandleInputReleased()
{
	const UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get());
	if (!WxASC)
	{
		return;
	}

	if (!WxASC->GetLastReleasedInputTag().MatchesTag(InputTag))
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnReleased.Broadcast();
	}
	EndTask();
}
