// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_TurnAround.h"

UWxAbilityTask_TurnAround* UWxAbilityTask_TurnAround::CreateTask(UGameplayAbility* OwningAbility, FVector InTargetDirection, float InInterpSpeed, float InCompletionThreshold)
{
	UWxAbilityTask_TurnAround* Task = NewAbilityTask<UWxAbilityTask_TurnAround>(OwningAbility);
	Task->TargetRotation = InTargetDirection.Rotation();
	Task->InterpSpeed = InInterpSpeed;
	Task->CompletionThresholdSquared = InCompletionThreshold * InCompletionThreshold;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_TurnAround::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		EndTask();
		return;
	}

	const FRotator CurrentRotation = Avatar->GetActorRotation();
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);
	Avatar->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));

	const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewRotation.Yaw, TargetRotation.Yaw);
	if (FMath::Square(DeltaYaw) < CompletionThresholdSquared)
	{
		OnCompleted.Broadcast();
		EndTask();
	}
}
