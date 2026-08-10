// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

UWxCharacterMovementComponent::UWxCharacterMovementComponent()
{
	JumpZVelocity = 640.f;
}

float UWxCharacterMovementComponent::GetGravityZ() const
{
	const float Multiplier = (Velocity.Z > 0.f) ? 2.f : 2.5f;

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
