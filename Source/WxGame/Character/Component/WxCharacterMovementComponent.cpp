// Copyright Woogle. All Rights Reserved.

#include "WxCharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

namespace
{
	const FName LandingSectionName = TEXT("Grounded");
}

UWxCharacterMovementComponent::UWxCharacterMovementComponent()
{
	bOrientRotationToMovement = true;
	RotationRate = FRotator(0.f, 500.f, 0.f);
	NavMovementProperties.bUseAccelerationForPaths = true;

	MaxAcceleration = 1500.f;
	MinAnalogWalkSpeed = 20.f;
	BrakingDecelerationWalking = 2000.f;
	bUseSeparateBrakingFriction = true;
	BrakingFrictionFactor = 1.f;
	
	JumpZVelocity = 640.f;
	GravityScale = 2.f;
	AirControl = 0.35f;
}

float UWxCharacterMovementComponent::GetGravityZ() const
{
	if (Velocity.Z >= 0.f)
	{
		return Super::GetGravityZ();
	}
	else
	{
		return Super::GetGravityZ() * 1.25f;
	}
}

UAbilitySystemComponent* UWxCharacterMovementComponent::GetAbilitySystemComponent()
{
	if (!AbilitySystemComponent && CharacterOwner)
	{
		AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner);
	}

	return AbilitySystemComponent;
}

void UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// 몽타주를 쓰지 않는 어빌리티는 앉은 자세와 공존한다.
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->GetAnimatingAbility())
		{
			bWantsToCrouch = false;
		}
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UWxCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (IsFalling())
		{
			ASC->SetLooseGameplayTagCount(WxGameplayTags::Movement_InAir, 1);
		}
		else
		{
			ASC->SetLooseGameplayTagCount(WxGameplayTags::Movement_InAir, 0);
		}
	}

	if (PreviousMovementMode == MOVE_Falling)
	{
		JumpToLandingSection();
	}
}

void UWxCharacterMovementComponent::JumpToLandingSection()
{
	if (!CharacterOwner)
	{
		return;
	}
	const USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh();
	if (!Mesh)
	{
		return;
	}
	
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}
	
	UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();

	if (Montage && Montage->GetSectionIndex(LandingSectionName) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(LandingSectionName, Montage);
	}
}
