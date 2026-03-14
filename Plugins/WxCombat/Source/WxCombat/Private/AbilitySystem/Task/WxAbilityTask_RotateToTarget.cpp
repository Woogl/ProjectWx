// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_RotateToTarget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWxAbilityTask_RotateToTarget* UWxAbilityTask_RotateToTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTargetActor)
{
	UWxAbilityTask_RotateToTarget* Task = NewAbilityTask<UWxAbilityTask_RotateToTarget>(OwningAbility);
	Task->TargetActor = InTargetActor;
	return Task;
}

void UWxAbilityTask_RotateToTarget::Activate()
{
	Super::Activate();

	bTickingTask = true;

	if (!TargetActor.IsValid())
	{
		EndTask();
	}
}

void UWxAbilityTask_RotateToTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	AActor* AvatarActor = GetAvatarActor();
	if (!AvatarActor || !TargetActor.IsValid())
	{
		EndTask();
		return;
	}

	FVector Direction = TargetActor->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0;
	if (Direction.IsNearlyZero())
	{
		EndTask();
		return;
	}

	const FRotator DesiredRotation = Direction.Rotation();
	const FRotator CurrentRotation = AvatarActor->GetActorRotation();

	const ACharacter* Character = Cast<ACharacter>(AvatarActor);
	const float RotationRateYaw = Character ? Character->GetCharacterMovement()->RotationRate.Yaw : 360.f;

	const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, DesiredRotation, DeltaTime, RotationRateYaw);
	AvatarActor->SetActorRotation(NewRotation);

	if (FMath::Abs(FRotator::NormalizeAxis(NewRotation.Yaw - DesiredRotation.Yaw)) <= RotationTolerance)
	{
		EndTask();
	}
}
