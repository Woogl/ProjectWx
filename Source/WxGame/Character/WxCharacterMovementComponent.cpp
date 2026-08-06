// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystem/Ability/WxAbility_LockOn.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

UWxCharacterMovementComponent::UWxCharacterMovementComponent()
{
	JumpZVelocity = 640.f;
}

float UWxCharacterMovementComponent::GetGravityZ() const
{
	// 상승 중일 때 2배, 하강 중일 때 2.5배 중력 스케일 적용
	const float Multiplier = (Velocity.Z < 0.f) ? 2.f : 2.5f;

	return Super::GetGravityZ() * Multiplier;
}

void UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	const AWxCharacterBase* WxCharacter = bWantsToCrouch ? Cast<AWxCharacterBase>(CharacterOwner) : nullptr;
	const UAbilitySystemComponent* ASC = WxCharacter ? WxCharacter->GetAbilitySystemComponent() : nullptr;
	if (ASC)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			// 락온은 토글형 조준이라 앉은 자세와 공존한다.
			if (Spec.IsActive() && !Spec.Ability->IsA<UWxAbility_LockOn>())
			{
				bWantsToCrouch = false;
				break;
			}
		}
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}
