// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "WxGameplayTags.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed, float InMaxPitchOffset, float InMaxDistance)
{
	UWxAbilityTask_LockOnTarget* Task = NewAbilityTask<UWxAbilityTask_LockOnTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->MaxPitchOffset = InMaxPitchOffset;
	Task->MaxDistanceSquared = InMaxDistance * InMaxDistance;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnTarget::Activate()
{
	Super::Activate();

	if (AActor* TargetActor = Target.Get())
	{
		TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged);
		}
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

	const FVector TargetLocation = TargetActor->GetActorLocation();

	// 거리 초과 시 락온 해제
	const float DistanceSquared = FVector::DistSquared(AvatarPawn->GetActorLocation(), TargetLocation);
	if (DistanceSquared > MaxDistanceSquared)
	{
		OnTargetLost.Broadcast();
		return;
	}

	APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
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

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_LockOnTarget::HandleTargetDestroyed(AActor* DestroyedActor)
{
	OnTargetLost.Broadcast();
}

void UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		OnTargetLost.Broadcast();
	}
}
