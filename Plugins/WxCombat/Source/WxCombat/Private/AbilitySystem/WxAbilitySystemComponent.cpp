// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "WxGameplayTags.h"

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

void UWxAbilitySystemComponent::MulticastEnableRagdoll_Implementation()
{
	ACharacter* Character = Cast<ACharacter>(GetOwnerActor());
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetSimulatePhysics(true);

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Character->GetCharacterMovement()->DisableMovement();
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
