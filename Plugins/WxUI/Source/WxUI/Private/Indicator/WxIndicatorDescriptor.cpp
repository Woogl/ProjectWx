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

void UWxIndicatorDescriptor::Initialize(UWxIndicatorManagerComponent* InOwningManager, const FVector& InWorldLocation, const FVector& InWorldOffset)
{
	check(!OwningManager.IsValid());

	OwningManager = InOwningManager;
	WorldLocation = InWorldLocation;
	WorldOffset = InWorldOffset;
}

USceneComponent* UWxIndicatorDescriptor::GetTargetComponent() const
{
	return TargetComponent;
}

FVector UWxIndicatorDescriptor::GetWorldLocation() const
{
	return WorldLocation;
}

FVector UWxIndicatorDescriptor::GetWorldOffset() const
{
	return WorldOffset;
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

bool UWxIndicatorDescriptor::IsProjected() const
{
	return bProjected;
}

FVector2D UWxIndicatorDescriptor::GetScreenPosition() const
{
	return ScreenPosition;
}

float UWxIndicatorDescriptor::GetDistanceMeters() const
{
	return DistanceMeters;
}

bool UWxIndicatorDescriptor::IsClamped() const
{
	return bClamped;
}

void UWxIndicatorDescriptor::Unregister()
{
	if (UWxIndicatorManagerComponent* Manager = OwningManager.Get())
	{
		Manager->RemoveIndicator(this);
	}
}
