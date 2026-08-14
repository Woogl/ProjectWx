// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "Character/WxCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

namespace
{
	/** 이 섹션을 가진 몽타주만 착지에 반응하므로 이름이 곧 프로젝트 공통 규약이다 */
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

void UWxCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	const AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(CharacterOwner);
	UAbilitySystemComponent* ASC = WxCharacter ? WxCharacter->GetAbilitySystemComponent() : nullptr;

	// 부여/제거 짝이 아니라 절대값이라 전이가 겹치거나 어긋나도 카운트가 새지 않는다.
	if (ASC)
	{
		ASC->SetLooseGameplayTagCount(WxGameplayTags::Movement_InAir, IsFalling() ? 1 : 0);
	}

	if (PreviousMovementMode == MOVE_Falling)
	{
		JumpToLandingSection();
	}
}

void UWxCharacterMovementComponent::JumpToLandingSection()
{
	const USkeletalMeshComponent* Mesh = CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;

	// 섹션이 없는 몽타주에 점프하면 엔진이 경고를 남기므로, 이 검사가 경고를 막으면서 착지 반응 여부도 가른다.
	if (Montage && Montage->GetSectionIndex(LandingSectionName) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(LandingSectionName, Montage);
	}
}
