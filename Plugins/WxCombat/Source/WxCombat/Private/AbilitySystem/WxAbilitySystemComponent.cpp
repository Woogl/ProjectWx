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

	SetLastPressedInputTag(InputTag);

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && InputTag.MatchesAny(Spec.GetDynamicSpecSourceTags()))
		{
			Spec.InputPressed = true;
			if (Spec.IsActive())
			{
				AbilitySpecInputPressed(Spec);

				for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
				{
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
				}
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

	SetLastReleasedInputTag(InputTag);

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability && InputTag.MatchesAny(Spec.GetDynamicSpecSourceTags()))
		{
			Spec.InputPressed = false;
			if (Spec.IsActive())
			{
				AbilitySpecInputReleased(Spec);

				for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
				{
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
				}
			}
		}
	}
}

const FGameplayTag& UWxAbilitySystemComponent::GetLastPressedInputTag() const
{
	return LastPressedInputTag;
}

const FGameplayTag& UWxAbilitySystemComponent::GetLastReleasedInputTag() const
{
	return LastReleasedInputTag;
}

void UWxAbilitySystemComponent::SetLastPressedInputTag(const FGameplayTag& InputTag)
{
	LastPressedInputTag = InputTag;

	if (!GetOwnerActor()->HasAuthority())
	{
		ServerSetLastPressedInputTag(InputTag);
	}
}

void UWxAbilitySystemComponent::SetLastReleasedInputTag(const FGameplayTag& InputTag)
{
	LastReleasedInputTag = InputTag;

	if (!GetOwnerActor()->HasAuthority())
	{
		ServerSetLastReleasedInputTag(InputTag);
	}
}

void UWxAbilitySystemComponent::ServerSetLastPressedInputTag_Implementation(const FGameplayTag& InputTag)
{
	LastPressedInputTag = InputTag;
}

void UWxAbilitySystemComponent::ServerSetLastReleasedInputTag_Implementation(const FGameplayTag& InputTag)
{
	LastReleasedInputTag = InputTag;
}
