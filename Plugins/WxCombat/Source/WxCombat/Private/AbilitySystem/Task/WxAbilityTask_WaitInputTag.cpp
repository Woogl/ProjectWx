// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_WaitInputTag.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

UWxAbilityTask_WaitInputTag* UWxAbilityTask_WaitInputTag::CreateTask(UGameplayAbility* OwningAbility, FGameplayTag InInputTag)
{
	UWxAbilityTask_WaitInputTag* Task = NewAbilityTask<UWxAbilityTask_WaitInputTag>(OwningAbility);
	Task->InputTag = InInputTag;
	return Task;
}

void UWxAbilityTask_WaitInputTag::Activate()
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
	).AddUObject(this, &UWxAbilityTask_WaitInputTag::HandleInputPressed);
}

void UWxAbilityTask_WaitInputTag::OnDestroy(bool AbilityEnded)
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

void UWxAbilityTask_WaitInputTag::HandleInputPressed()
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
