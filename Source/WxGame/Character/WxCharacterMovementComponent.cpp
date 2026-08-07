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
	// 상승 중일 때 2배, 하강 중일 때 2.5배 중력 스케일 적용
	const float Multiplier = (Velocity.Z > 0.f) ? 2.f : 2.5f;

	return Super::GetGravityZ() * Multiplier;
}

void UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	const AWxCharacterBase* WxCharacter = bWantsToCrouch ? Cast<AWxCharacterBase>(CharacterOwner) : nullptr;
	const UAbilitySystemComponent* ASC = WxCharacter ? WxCharacter->GetAbilitySystemComponent() : nullptr;

	// 어빌리티 몽타주가 재생 중이면 앉지 않는다. 차단이 풀리는 후딜 구간에도 몽타주는 이어지므로 재생 도중 자세가 바뀌지 않는다.
	// 락온·스프린트·상호작용처럼 몽타주를 쓰지 않는 어빌리티는 앉은 자세와 공존한다.
	if (ASC && ASC->GetAnimatingAbility())
	{
		bWantsToCrouch = false;
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}
