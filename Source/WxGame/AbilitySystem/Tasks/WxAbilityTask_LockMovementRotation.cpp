// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/WxAbilityTask_LockMovementRotation.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWxAbilityTask_LockMovementRotation* UWxAbilityTask_LockMovementRotation::CreateTask(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UWxAbilityTask_LockMovementRotation>(OwningAbility);
}

void UWxAbilityTask_LockMovementRotation::Activate()
{
	Super::Activate();
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		EndTask();
		return;
	}
	LockedMovement = Movement;
	const UWxAbilityTask_LockMovementRotation* Existing = FindOtherLock();
	SavedRotationRate = Existing ? Existing->SavedRotationRate : Movement->RotationRate;
	// 회전 모드 플래그는 락온 등 다른 기능이 변경할 수 있으므로 건드리지 않는다.
	Movement->RotationRate = FRotator::ZeroRotator;
}

UWxAbilityTask_LockMovementRotation* UWxAbilityTask_LockMovementRotation::FindOtherLock() const
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC)
	{
		for (auto It = ASC->GetKnownTaskIterator(); It; ++It)
		{
			UWxAbilityTask_LockMovementRotation* Other = Cast<UWxAbilityTask_LockMovementRotation>(*It);
			if (Other && Other != this && Other->LockedMovement.IsValid() && Other->LockedMovement == LockedMovement)
			{
				return Other;
			}
		}
	}
	return nullptr;
}

void UWxAbilityTask_LockMovementRotation::OnDestroy(bool AbilityIsEnding)
{
	if (UCharacterMovementComponent* Movement = LockedMovement.Get(); Movement && !FindOtherLock())
	{
		Movement->RotationRate = SavedRotationRate;
	}
	LockedMovement.Reset();
	Super::OnDestroy(AbilityIsEnding);
}
