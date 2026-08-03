// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicatorDescriptor.h"

#include "Components/SceneComponent.h"
#include "Indicator/WxIndicatorManagerComponent.h"

void UWxIndicatorDescriptor::Initialize(UWxIndicatorManagerComponent* InOwningManager, USceneComponent* InTargetComponent, const FVector& InWorldOffset)
{
	check(!OwningManager.IsValid());

	OwningManager = InOwningManager;
	TargetComponent = InTargetComponent;
	WorldOffset = InWorldOffset;
}

void UWxIndicatorDescriptor::SetProjection(const FVector2D& InScreenPosition, float InDistanceMeters, bool bInClamped)
{
	ScreenPosition = InScreenPosition;
	DistanceMeters = InDistanceMeters;
	bClamped = bInClamped;
	bProjected = true;
}

void UWxIndicatorDescriptor::ClearProjection()
{
	bProjected = false;
}

void UWxIndicatorDescriptor::Unregister()
{
	if (UWxIndicatorManagerComponent* Manager = OwningManager.Get())
	{
		Manager->RemoveIndicator(this);
	}
}
