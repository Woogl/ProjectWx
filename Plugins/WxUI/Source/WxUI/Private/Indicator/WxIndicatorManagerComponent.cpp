// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicatorManagerComponent.h"

#include "Components/SceneComponent.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "Indicator/WxIndicatorWidget.h"

UWxIndicatorDescriptor* UWxIndicatorManagerComponent::AddIndicator(USceneComponent* InTargetComponent, const TSoftClassPtr<UWxIndicatorWidget>& InIndicatorWidgetClass, const FVector& InWorldOffset)
{
	if (!IsValid(InTargetComponent) || InIndicatorWidgetClass.IsNull())
	{
		return nullptr;
	}

	UWxIndicatorDescriptor* Indicator = NewObject<UWxIndicatorDescriptor>(this);
	Indicator->Initialize(this, InTargetComponent, InIndicatorWidgetClass, InWorldOffset);

	Indicators.Add(Indicator);
	OnIndicatorAdded.Broadcast(Indicator);

	return Indicator;
}

void UWxIndicatorManagerComponent::RemoveIndicator(UWxIndicatorDescriptor* Indicator)
{
	if (Indicators.Remove(Indicator) > 0)
	{
		OnIndicatorRemoved.Broadcast(Indicator);
	}
}
