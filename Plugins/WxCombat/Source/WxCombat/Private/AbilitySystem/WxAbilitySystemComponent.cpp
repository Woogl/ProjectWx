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

void UWxAbilitySystemComponent::AbilityInputActionStarted(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	SetLastPressedInputAction(Action);

	// 활성 어빌리티의 입력 대기 태스크가 관심 입력을 스스로 필터링한다(가드/회피의 반격 등).
	// 누른 순간에만 띄운다. 반격 윈도우는 새로 누른 입력에만 반응해야지, 윈도우가 열리기 전부터 쥐고 있던 키에 즉시 반응하면 안 된다.
	OnInputActionTriggeredDelegate.Broadcast(Action);

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability)
		{
			continue;
		}

		if (!Ability->IsActivationInput(Action))
		{
			continue;
		}

		// 아직 돌고 있지 않은 어빌리티의 발동은 AbilityInputActionTriggered가 맡는다.
		if (!Spec.IsActive())
		{
			continue;
		}

		// 활성 어빌리티에 자기 발동 입력이 다시 들어온 경우다(반격 등 남의 입력은 위 방송으로 처리).
		// 콤보처럼 재발동 가능한 어빌리티는 TryActivateAbility가 다음 단계로 진행시킨다.
		// 재발동이 성립하지 않으면 활성 인스턴스에 InputPressed 이벤트를 전달한다(예: 가드 버튼 재입력으로 패링 중 가드 복귀).
		Spec.InputPressed = true;
		if (!TryActivateAbility(Spec.Handle))
		{
			AbilitySpecInputPressed(Spec);

			for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

void UWxAbilitySystemComponent::AbilityInputActionTriggered(const UInputAction* Action)
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

		if (!Ability->IsActivationInput(Action))
		{
			continue;
		}

		// 활성 어빌리티에 들어온 재입력은 AbilityInputActionStarted가 맡는다.
		if (Spec.IsActive())
		{
			continue;
		}

		Spec.InputPressed = true;
		if (TryActivateAbility(Spec.Handle))
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

		if (!Ability->IsActivationInput(Action))
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

TArray<const UInputAction*> UWxAbilitySystemComponent::GetAbilityInputActions() const
{
	if (AbilitySet)
	{
		return AbilitySet->GetInputActions();
	}
	return TArray<const UInputAction*>();
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
