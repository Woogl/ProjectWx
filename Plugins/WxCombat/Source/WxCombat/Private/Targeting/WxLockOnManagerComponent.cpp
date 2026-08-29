// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxLockOnManagerComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
#include "Targeting/WxLockOnPointComponent.h"

UWxLockOnManagerComponent::UWxLockOnManagerComponent()
{
	SetIsReplicatedByDefault(true);
}

UWxLockOnManagerComponent* UWxLockOnManagerComponent::FindComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UWxLockOnManagerComponent>();
}

void UWxLockOnManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxLockOnManagerComponent, LockOnTarget);
}

void UWxLockOnManagerComponent::SetLockOnTarget(USceneComponent* InTarget)
{
	ApplyLockOnTarget(InTarget);

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerSetLockOnTarget(InTarget);
	}
}

void UWxLockOnManagerComponent::ServerSetLockOnTarget_Implementation(USceneComponent* InTarget)
{
	if (!InTarget)
	{
		ApplyLockOnTarget(InTarget);
		return;
	}

	const UWxLockOnPointComponent* TargetPoint = Cast<UWxLockOnPointComponent>(InTarget);
	const AActor* TargetActor = InTarget->GetOwner();
	if (TargetPoint && TargetActor && TargetPoint->CanBeLockedOn())
	{
		ApplyLockOnTarget(InTarget);
	}
}

void UWxLockOnManagerComponent::OnRep_LockOnTarget()
{
	OnLockOnTargetChanged.Broadcast(LockOnTarget);
}

void UWxLockOnManagerComponent::ApplyLockOnTarget(USceneComponent* InTarget)
{
	if (LockOnTarget == InTarget)
	{
		return;
	}

	LockOnTarget = InTarget;

	// 권위 측은 OnRep이 불리지 않으므로 여기서 직접 브로드캐스트한다.
	OnLockOnTargetChanged.Broadcast(LockOnTarget);
}

USceneComponent* UWxLockOnManagerComponent::GetLockOnTarget() const
{
	return LockOnTarget;
}

void UWxLockOnManagerComponent::SetLookInput(const FVector2D& InLookInput)
{
	LookInput = InLookInput;
}

FVector2D UWxLockOnManagerComponent::ConsumeLookInput()
{
	// 시선 입력은 있는 프레임에만 들어오므로, 비우지 않으면 손을 뗀 뒤에도 마지막 값이 계속 읽힌다.
	const FVector2D Consumed = LookInput;
	LookInput = FVector2D::ZeroVector;
	return Consumed;
}
