// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicatorDescriptor.h"

#include "Components/SceneComponent.h"
#include "Indicator/WxIndicatorManagerComponent.h"

void UWxIndicatorDescriptor::Initialize(UWxIndicatorManagerComponent* InOwningManager, USceneComponent* InTargetComponent, const TSoftClassPtr<UWxIndicatorWidget>& InIndicatorWidgetClass, const FVector& InWorldOffset)
{
	check(!OwningManager.IsValid());

	OwningManager = InOwningManager;
	TargetComponent = InTargetComponent;
	IndicatorWidgetClass = InIndicatorWidgetClass;
	WorldOffset = InWorldOffset;
}

void UWxIndicatorDescriptor::Unregister()
{
	if (UWxIndicatorManagerComponent* Manager = OwningManager.Get())
	{
		Manager->RemoveIndicator(this);
	}
}
