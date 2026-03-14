// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_RotateToTarget.h"

UWxAbilityTask_RotateToTarget::UWxAbilityTask_RotateToTarget()
	: RotationRateYaw(360.f)
{
	bTickingTask = true;
}

UWxAbilityTask_RotateToTarget* UWxAbilityTask_RotateToTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTargetActor, float InRotationRateYaw)
{
	UWxAbilityTask_RotateToTarget* Task = NewAbilityTask<UWxAbilityTask_RotateToTarget>(OwningAbility);
	Task->TargetActor = InTargetActor;
	Task->RotationRateYaw = InRotationRateYaw;
	return Task;
}

void UWxAbilityTask_RotateToTarget::Activate()
{
	Super::Activate();

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

	const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, DesiredRotation, DeltaTime, RotationRateYaw);
	AvatarActor->SetActorRotation(NewRotation);

	if (FMath::Abs(FRotator::NormalizeAxis(NewRotation.Yaw - DesiredRotation.Yaw)) <= RotationTolerance)
	{
		EndTask();
	}
}
