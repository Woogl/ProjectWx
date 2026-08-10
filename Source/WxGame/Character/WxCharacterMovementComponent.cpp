// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

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
	const float Multiplier = (Velocity.Z > 0.f) ? 1.f : 1.25f;

	return Super::GetGravityZ() * Multiplier;
}

void UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	const AWxCharacterBase* WxCharacter = bWantsToCrouch ? Cast<AWxCharacterBase>(CharacterOwner) : nullptr;
	const UAbilitySystemComponent* ASC = WxCharacter ? WxCharacter->GetAbilitySystemComponent() : nullptr;

	// 차단이 풀리는 후딜 구간에도 몽타주는 이어지므로 재생 도중 자세가 바뀌지 않는다.
	if (ASC && ASC->GetAnimatingAbility())
	{
		bWantsToCrouch = false;
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}
