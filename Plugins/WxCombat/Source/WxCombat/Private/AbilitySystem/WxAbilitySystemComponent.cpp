// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

FWxInputActionTriggeredSignature& UWxAbilitySystemComponent::OnInputActionTriggered()
{
	return OnInputActionTriggeredDelegate;
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
}

void UWxAbilitySystemComponent::AbilityInputActionTriggered(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	// 순회 중 활성화가 어빌리티 목록을 바꿀 수 있다(GE의 GrantedAbilities, RemoveAfterActivation 등).
	// 락이 없으면 Give/Clear가 ActivatableAbilities.Items를 즉시 Add/RemoveAtSwap 해 참조와 이터레이터가 무효화된다.
	// 엔진의 AbilityLocalInputPressed도 같은 이유로 이 락을 건다.
	ABILITYLIST_SCOPE_LOCK();

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

		// 신규 발동과 콤보 재발동은 엔진이 bRetriggerInstancedAbility로 가르므로 호출이 같다.
		if (TryActivateAbility(Spec.Handle))
		{
			break;
		}

		// 재발동을 받지 않는 어빌리티는 활성 인스턴스가 입력을 직접 처리한다(락온 토글 해제, 패링 중 가드 복귀).
		if (Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);

			for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Instance->GetCurrentActivationInfo().GetActivationPredictionKey());
			}
		}
	}
}

void UWxAbilitySystemComponent::AbilityInputActionReleased(const UInputAction* Action)
{
	if (!Action)
	{
		return;
	}

	// AbilityInputActionTriggered와 같은 이유로 락을 건다(엔진의 AbilityLocalInputReleased와 동일).
	ABILITYLIST_SCOPE_LOCK();

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
