// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
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
	// 오너의 ASC 는 생성자 서브오브젝트라 한 번 잡으면 바뀌지 않는다.
	// 오너가 정해지는 시점(등록)과 첫 호출 시점(PostInitializeComponents 의 초기 이동 모드 설정)이 엇갈릴 수 있어 초기화 훅 대신 지연 해석한다.
	if (!AbilitySystemComponent && CharacterOwner)
	{
		AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner);
	}

	return AbilitySystemComponent;
}

void UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
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
