// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnCamera.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxLockOnPointComponent.h"

UWxAbilityTask_LockOnCamera* UWxAbilityTask_LockOnCamera::CreateTask(UGameplayAbility* OwningAbility, float InInterpSpeed, float InPitchOffset, float InMaxDistance, float InRetargetLookThreshold)
{
	UWxAbilityTask_LockOnCamera* Task = NewAbilityTask<UWxAbilityTask_LockOnCamera>(OwningAbility);
	Task->InterpSpeed = InInterpSpeed;
	Task->PitchOffset = InPitchOffset;
	Task->MaxDistanceSquared = InMaxDistance * InMaxDistance;
	Task->RetargetLookThreshold = InRetargetLookThreshold;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnCamera::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	UWxLockOnComponent* Comp = LockOnComponent.Get();
	USceneComponent* TargetComponent = Comp ? Comp->GetLockOnTarget() : nullptr;
	if (!TargetComponent)
	{
		// 대상 액터나 추적 중인 부위 컴포넌트가 파괴돼도 GetLockOnTarget이 널로 답해 여기서 락온이 해제된다.
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnTargetLost.Broadcast();
		}

		return;
	}

	// 사망 등 태그 조건 상실도 거리·널 상실과 같이 폴링으로 감지한다.
	const UWxLockOnPointComponent* TargetPoint = Cast<UWxLockOnPointComponent>(TargetComponent);
	if (TargetPoint && !TargetPoint->CanBeLockedOn())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnTargetLost.Broadcast();
		}

		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	const FVector TargetLocation = TargetComponent->GetComponentLocation();

	const float DistanceSquared = FVector::DistSquared(AvatarPawn->GetActorLocation(), TargetLocation);
	if (DistanceSquared > MaxDistanceSquared)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnTargetLost.Broadcast();
		}

		return;
	}

	APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());
	if (!PC)
	{
		return;
	}

	const FRotator LookAtRotation = (TargetLocation - AvatarPawn->GetActorLocation()).Rotation();

	FRotator DesiredControlRotation = LookAtRotation;
	DesiredControlRotation.Pitch += PitchOffset;
	const FRotator NewControlRotation = FMath::RInterpTo(PC->GetControlRotation(), DesiredControlRotation, DeltaTime, InterpSpeed);
	PC->SetControlRotation(NewControlRotation);

	const FVector2D LookAxis = Comp->ConsumeLookInput();
	if (LookAxis.IsNearlyZero())
	{
		// 입력이 없는 프레임에는 누적을 초기화해, 띄엄띄엄 들어온 입력이 아니라 한 번의 큰 시선 이동만 묶는다.
		AccumulatedLook = FVector2D::ZeroVector;
		return;
	}

	AccumulatedLook += LookAxis;
	if (AccumulatedLook.Size() >= RetargetLookThreshold)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnRetargetRequested.Broadcast(AccumulatedLook.GetSafeNormal());
		}

		AccumulatedLook = FVector2D::ZeroVector;
	}
}

void UWxAbilityTask_LockOnCamera::Activate()
{
	Super::Activate();

	const AActor* Avatar = GetAvatarActor();
	LockOnComponent = Avatar ? Avatar->FindComponentByClass<UWxLockOnComponent>() : nullptr;
}
