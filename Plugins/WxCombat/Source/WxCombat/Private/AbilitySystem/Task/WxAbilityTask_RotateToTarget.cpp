// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_RotateToTarget.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"

UWxAbilityTask_RotateToTarget* UWxAbilityTask_RotateToTarget::CreateTask(UGameplayAbility* OwningAbility, USceneComponent* InTarget, float InInterpSpeed)
{
	UWxAbilityTask_RotateToTarget* Task = NewAbilityTask<UWxAbilityTask_RotateToTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_RotateToTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	const USceneComponent* TargetComponent = Target.Get();
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (!TargetComponent || !Character)
	{
		return;
	}

	FVector Direction = TargetComponent->GetComponentLocation() - Character->GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation(0.f, Direction.Rotation().Yaw, 0.f);
	const FRotator NewRotation = FMath::RInterpTo(Character->GetActorRotation(), DesiredRotation, DeltaTime, InterpSpeed);
	Character->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
}
