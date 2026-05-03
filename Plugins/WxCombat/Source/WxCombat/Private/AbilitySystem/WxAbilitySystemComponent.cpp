// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"

UWxAbilitySystemComponent::UWxAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWxAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxAbilitySystemComponent, bRagdollActive);
}

void UWxAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 래그돌이 켜진 채로 컴포넌트가 unregister되면 UActorComponent의 ensure가 트립한다.
	// ASC가 ragdoll 상태의 단일 진입점이므로 여기서 메시 시뮬레이션을 끈다.
	if (ACharacter* Character = Cast<ACharacter>(GetOwnerActor()))
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (Mesh && Mesh->IsSimulatingPhysics())
		{
			Mesh->SetSimulatePhysics(false);
		}
	}

	Super::EndPlay(EndPlayReason);
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

void UWxAbilitySystemComponent::SetRagdollActive(bool bNewActive)
{
	const AActor* Owner = GetOwnerActor();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (bRagdollActive == bNewActive)
	{
		return;
	}

	bRagdollActive = bNewActive;
	// 서버에서는 ReplicatedUsing 콜백이 자동 호출되지 않으므로 직접 발화한다.
	OnRep_RagdollActive();
}

void UWxAbilitySystemComponent::OnRep_RagdollActive()
{
	if (!bRagdollActive)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwnerActor());
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	// Ragdoll 프로필이 Camera 응답을 Block으로 덮어쓰므로, 스프링암 카메라가 래그돌 본에 걸려 줌-인되는 현상을 방지한다.
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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

const FGameplayTag& UWxAbilitySystemComponent::GetLastReleasedInputTag() const
{
	return LastReleasedInputTag;
}

void UWxAbilitySystemComponent::SetLastReleasedInputTag(const FGameplayTag& InputTag)
{
	LastReleasedInputTag = InputTag;

	if (!GetOwnerActor()->HasAuthority())
	{
		ServerSetLastReleasedInputTag(InputTag);
	}
}

void UWxAbilitySystemComponent::ServerSetLastReleasedInputTag_Implementation(const FGameplayTag& InputTag)
{
	LastReleasedInputTag = InputTag;
}
