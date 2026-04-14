// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_WaitInputTagPressed.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

UWxAbilityTask_WaitInputTagPressed* UWxAbilityTask_WaitInputTagPressed::CreateTask(UGameplayAbility* OwningAbility, FGameplayTag InInputTag)
{
	UWxAbilityTask_WaitInputTagPressed* Task = NewAbilityTask<UWxAbilityTask_WaitInputTagPressed>(OwningAbility);
	Task->InputTag = InInputTag;
	return Task;
}

void UWxAbilityTask_WaitInputTagPressed::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !Ability || !IsLocallyControlled())
	{
		EndTask();
		return;
	}

	DelegateHandle = ASC->AbilityReplicatedEventDelegate(
		EAbilityGenericReplicatedEvent::InputPressed,
		GetAbilitySpecHandle(),
		GetActivationPredictionKey()
	).AddUObject(this, &UWxAbilityTask_WaitInputTagPressed::HandleInputPressed);
}

void UWxAbilityTask_WaitInputTagPressed::OnDestroy(bool AbilityEnded)
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->AbilityReplicatedEventDelegate(
			EAbilityGenericReplicatedEvent::InputPressed,
			GetAbilitySpecHandle(),
			GetActivationPredictionKey()
		).Remove(DelegateHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UWxAbilityTask_WaitInputTagPressed::HandleInputPressed()
{
	const UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get());
	if (!WxASC)
	{
		return;
	}

	if (!WxASC->GetLastPressedInputTag().MatchesTag(InputTag))
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPressed.Broadcast();
	}
	EndTask();
}
