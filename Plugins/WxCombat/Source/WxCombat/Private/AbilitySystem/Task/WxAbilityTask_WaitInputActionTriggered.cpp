// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_WaitInputActionTriggered.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"

UWxAbilityTask_WaitInputActionTriggered* UWxAbilityTask_WaitInputActionTriggered::CreateTask(UGameplayAbility* OwningAbility, const UInputAction* InInputAction)
{
	UWxAbilityTask_WaitInputActionTriggered* Task = NewAbilityTask<UWxAbilityTask_WaitInputActionTriggered>(OwningAbility);
	Task->InputAction = InInputAction;
	return Task;
}

void UWxAbilityTask_WaitInputActionTriggered::OnDestroy(bool AbilityEnded)
{
	if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get()))
	{
		WxASC->OnInputActionTriggered().Remove(DelegateHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UWxAbilityTask_WaitInputActionTriggered::Activate()
{
	UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get());
	if (!WxASC || !Ability || !IsLocallyControlled())
	{
		EndTask();
		return;
	}

	DelegateHandle = WxASC->OnInputActionTriggered().AddUObject(this, &UWxAbilityTask_WaitInputActionTriggered::HandleInputTriggered);
}

void UWxAbilityTask_WaitInputActionTriggered::HandleInputTriggered(const UInputAction* Action)
{
	if (Action != InputAction)
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnTriggered.Broadcast();
	}
	EndTask();
}
