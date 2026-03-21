// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed, float InMaxPitchOffset)
{
	UWxAbilityTask_LockOnTarget* Task = NewAbilityTask<UWxAbilityTask_LockOnTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->MaxPitchOffset = InMaxPitchOffset;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnTarget::Activate()
{
	Super::Activate();

	if (AActor* TargetActor = Target.Get())
	{
		// TODO: 대상과 거리가 너무 멀어지거나 대상이 죽었을 때에도 타겟팅 해제되어야함
		TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);
	}
}

void UWxAbilityTask_LockOnTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	AActor* TargetActor = Target.Get();
	if (!TargetActor)
	{
		OnTargetLost.Broadcast();
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FRotator DesiredRotation = (TargetLocation - CameraLocation).Rotation();

	const FRotator CurrentRotation = PC->GetControlRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, InterpSpeed);

	// 피치 제한
	NewRotation.Pitch = FMath::ClampAngle(NewRotation.Pitch, -MaxPitchOffset, MaxPitchOffset);

	PC->SetControlRotation(NewRotation);
}

void UWxAbilityTask_LockOnTarget::OnDestroy(bool bInOwnerFinished)
{
	if (AActor* TargetActor = Target.Get())
	{
		TargetActor->OnDestroyed.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_LockOnTarget::HandleTargetDestroyed(AActor* DestroyedActor)
{
	OnTargetLost.Broadcast();
}
