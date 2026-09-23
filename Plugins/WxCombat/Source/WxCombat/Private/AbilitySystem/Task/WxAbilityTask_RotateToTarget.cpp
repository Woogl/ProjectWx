// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_RotateToTarget.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "Targeting/WxLockOnComponent.h"

UWxAbilityTask_RotateToTarget* UWxAbilityTask_RotateToTarget::CreateTask(UGameplayAbility* OwningAbility, float InInterpSpeed)
{
	UWxAbilityTask_RotateToTarget* Task = NewAbilityTask<UWxAbilityTask_RotateToTarget>(OwningAbility);
	Task->InterpSpeed = InInterpSpeed;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_RotateToTarget::Activate()
{
	Super::Activate();

	const AActor* Avatar = GetAvatarActor();
	LockOnComponent = Avatar ? Avatar->FindComponentByClass<UWxLockOnComponent>() : nullptr;
}

void UWxAbilityTask_RotateToTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	const UWxLockOnComponent* Comp = LockOnComponent.Get();
	const USceneComponent* TargetComponent = Comp ? Comp->GetLockOnTarget() : nullptr;
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
