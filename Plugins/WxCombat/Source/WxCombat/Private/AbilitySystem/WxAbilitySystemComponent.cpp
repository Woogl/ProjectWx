// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWxAbilitySystemComponent::GiveAbilitySet()
{
	if (!AbilitySet)
	{
		return;
	}

	AbilitySet->GiveToAbilitySystem(this, &AbilitySetGrantedHandles);
}

void UWxAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = true;
			if (Spec.IsActive())
			{
				AbilitySpecInputPressed(Spec);
			}
			else if (TryActivateAbility(Spec.Handle))
			{
				break;
			}
		}
	}
}

void UWxAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			Spec.InputPressed = false;
			if (Spec.IsActive())
			{
				AbilitySpecInputReleased(Spec);
			}
		}
	}
}
