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
				// 콤보처럼 재발동 가능한 어빌리티는 TryActivateAbility가 다음 단계로 진행시킨다.
				// 재발동이 성립하지 않는 어빌리티(가드/회피 카운터 등)에서만 활성 인스턴스로 InputPressed 이벤트를 전달한다.
				if (!TryActivateAbility(Spec.Handle))
				{
					AbilitySpecInputPressed(Spec);

					for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
					{
						InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
					}
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

void UWxAbilitySystemComponent::SetLastPressedInputTag(const FGameplayTag& InputTag)
{
	LastPressedInputTag = InputTag;

	if (!GetOwnerActor()->HasAuthority())
	{
		ServerSetLastPressedInputTag(InputTag);
	}
}

void UWxAbilitySystemComponent::ServerSetLastPressedInputTag_Implementation(const FGameplayTag& InputTag)
{
	LastPressedInputTag = InputTag;
}
