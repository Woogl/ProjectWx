// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxLockOnComponent.h"

UWxLockOnComponent* UWxLockOnComponent::FindComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UWxLockOnComponent>();
}

void UWxLockOnComponent::SetLockOnTarget(AActor* InTarget)
{
	LockOnTarget = InTarget;
}

AActor* UWxLockOnComponent::GetLockOnTarget() const
{
	return LockOnTarget.Get();
}
