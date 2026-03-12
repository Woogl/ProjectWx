// Copyright Woogle. All Rights Reserved.

#include "Component/WxRagdollComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

UWxRagdollComponent::UWxRagdollComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UWxRagdollComponent::IsRagdollActive() const
{
	return bRagdollActive;
}

void UWxRagdollComponent::EnableRagdoll()
{
	if (bRagdollActive)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	bRagdollActive = true;

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetSimulatePhysics(true);
	Mesh->WakeAllRigidBodies();

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Character->GetCharacterMovement()->DisableMovement();
}

void UWxRagdollComponent::DisableRagdoll()
{
	if (!bRagdollActive)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	bRagdollActive = false;

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	Mesh->SetSimulatePhysics(false);
	Mesh->SetAllBodiesSimulatePhysics(false);
	Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));

	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
