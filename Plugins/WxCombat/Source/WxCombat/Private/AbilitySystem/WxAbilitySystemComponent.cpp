// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"

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

void UWxAbilitySystemComponent::AbilityInputActionPressed(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	SetLastPressedInputAction(Action);

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability)
		{
			continue;
		}

		// 비활성이면 발동 입력만, 활성이면 발동 입력 + 관찰 입력(가드/회피의 반격 차용)까지 받는다.
		bool bMatch = Ability->IsActivationInput(Action);
		if (!bMatch && Spec.IsActive())
		{
			bMatch = Ability->IsObservedInput(Action);
		}
		if (!bMatch)
		{
			continue;
		}

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

void UWxAbilitySystemComponent::AbilityInputActionReleased(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability)
		{
			continue;
		}

		// 비활성이면 발동 입력만, 활성이면 발동 입력 + 관찰 입력(가드/회피의 반격 차용)까지 받는다.
		bool bMatch = Ability->IsActivationInput(Action);
		if (!bMatch && Spec.IsActive())
		{
			bMatch = Ability->IsObservedInput(Action);
		}
		if (!bMatch)
		{
			continue;
		}

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

const UInputAction* UWxAbilitySystemComponent::GetLastPressedInputAction() const
{
	return LastPressedInputAction;
}

void UWxAbilitySystemComponent::SetLastPressedInputAction(const UInputAction* Action)
{
	LastPressedInputAction = Action;

	if (!GetOwnerActor()->HasAuthority())
	{
		ServerSetLastPressedInputAction(Action);
	}
}

void UWxAbilitySystemComponent::ServerSetLastPressedInputAction_Implementation(const UInputAction* Action)
{
	LastPressedInputAction = Action;
}
