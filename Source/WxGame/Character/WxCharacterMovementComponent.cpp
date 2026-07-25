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
	// Super::GetGravityZ() == 월드 중력 * GravityScale(=1.0 유지) == 월드 중력값.
	// 여기에 상승/하강 스케일만 곱해 비대칭 중력을 적용한다.
	const float BaseGravityZ = Super::GetGravityZ();
	const float DirectionalScale = (Velocity.Z < 0.f) ? FallGravityScale : RiseGravityScale;
	return BaseGravityZ * DirectionalScale;
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
