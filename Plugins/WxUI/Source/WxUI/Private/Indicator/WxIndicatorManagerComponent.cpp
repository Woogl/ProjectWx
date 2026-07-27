// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicatorManagerComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "Indicator/WxIndicatorWidget.h"

UWxIndicatorDescriptor* UWxIndicatorManagerComponent::AddIndicator(USceneComponent* InTargetComponent, const TSoftClassPtr<UWxIndicatorWidget>& InIndicatorWidgetClass, const FVector& InWorldOffset)
{
	if (!IsValid(InTargetComponent) || InIndicatorWidgetClass.IsNull())
	{
		return nullptr;
	}

	// 화면이 있는 쪽에서만 의미가 있으므로 원격 사본(데디 서버가 들고 있는 PC 등)은 발급하지 않는다.
	const APlayerController* OwningController = GetController<APlayerController>();
	if (!OwningController || !OwningController->IsLocalController())
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
