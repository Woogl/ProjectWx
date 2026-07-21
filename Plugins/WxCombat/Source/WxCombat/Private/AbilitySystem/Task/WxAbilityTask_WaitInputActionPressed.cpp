// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_WaitInputActionPressed.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

UWxAbilityTask_WaitInputActionPressed* UWxAbilityTask_WaitInputActionPressed::CreateTask(UGameplayAbility* OwningAbility, const UInputAction* InInputAction)
{
	UWxAbilityTask_WaitInputActionPressed* Task = NewAbilityTask<UWxAbilityTask_WaitInputActionPressed>(OwningAbility);
	Task->InputAction = InInputAction;
	return Task;
}

void UWxAbilityTask_WaitInputActionPressed::OnDestroy(bool AbilityEnded)
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

void UWxAbilityTask_WaitInputActionPressed::Activate()
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
	).AddUObject(this, &UWxAbilityTask_WaitInputActionPressed::HandleInputPressed);
}

void UWxAbilityTask_WaitInputActionPressed::HandleInputPressed()
{
	const UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get());
	if (!WxASC)
	{
		return;
	}

	if (WxASC->GetLastPressedInputAction() != InputAction)
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPressed.Broadcast();
	}
	EndTask();
}
