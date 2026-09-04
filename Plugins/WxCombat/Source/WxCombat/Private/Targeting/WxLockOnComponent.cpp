// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxLockOnComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UWxLockOnComponent::UWxLockOnComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWxLockOnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxLockOnComponent, LockOnTarget);
}

AActor* UWxLockOnComponent::ResolveLockOnTargetActor(const AActor* Source)
{
	const UWxLockOnComponent* LockOnComponent = Source ? Source->FindComponentByClass<UWxLockOnComponent>() : nullptr;
	const USceneComponent* Target = LockOnComponent ? LockOnComponent->GetLockOnTarget() : nullptr;
	return Target ? Target->GetOwner() : nullptr;
}

void UWxLockOnComponent::SetLockOnTarget(USceneComponent* InTarget)
{
	ApplyLockOnTarget(InTarget);

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerSetLockOnTarget(InTarget);
	}
}

void UWxLockOnComponent::ServerSetLockOnTarget_Implementation(USceneComponent* InTarget)
{
	// 조건부로 무시하면 이미 로컬에 반영된 클라 값만 남고 복제로도 정정되지 않으므로, 해제(nullptr)까지 그대로 받는다.
	ApplyLockOnTarget(InTarget);
}

void UWxLockOnComponent::OnRep_LockOnTarget()
{
	OnLockOnTargetChanged.Broadcast(LockOnTarget);
}

void UWxLockOnComponent::ApplyLockOnTarget(USceneComponent* InTarget)
{
	if (LockOnTarget == InTarget)
	{
		return;
	}

	LockOnTarget = InTarget;

	// 권위 측은 OnRep이 불리지 않으므로 여기서 직접 브로드캐스트한다.
	OnLockOnTargetChanged.Broadcast(LockOnTarget);
}

USceneComponent* UWxLockOnComponent::GetLockOnTarget() const
{
	// 파괴된 대상은 다음 GC 까지 하드 참조에 남는다.
	// 서버는 대상 소실 경로가 즉시 비우지만 그 경로가 없는 클라는 복제된 널이 올 때까지 죽은 컴포넌트를 읽게 되므로, 읽는 이 길목에서 끊는다.
	return IsValid(LockOnTarget) ? LockOnTarget.Get() : nullptr;
}

void UWxLockOnComponent::SetLookInput(const FVector2D& InLookInput)
{
	LookInput = InLookInput;
}

FVector2D UWxLockOnComponent::ConsumeLookInput()
{
	// 시선 입력은 있는 프레임에만 들어오므로, 비우지 않으면 손을 뗀 뒤에도 마지막 값이 계속 읽힌다.
	const FVector2D Consumed = LookInput;
	LookInput = FVector2D::ZeroVector;
	return Consumed;
}
