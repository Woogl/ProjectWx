// Copyright Woogle. All Rights Reserved.

#include "WxUILibrary.h"
#include "System/WxUIManagerSubsystem.h"
#include "System/WxPrimaryGameLayout.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UWxUIManagerSubsystem* UWxUILibrary::GetUIManagerSubsystem(const UObject* WorldContextObject)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GameInstance)
	{
		return nullptr;
	}
	return GameInstance->GetSubsystem<UWxUIManagerSubsystem>();
}

UWxPrimaryGameLayout* UWxUILibrary::GetPrimaryGameLayout(const UObject* WorldContextObject)
{
	UWxUIManagerSubsystem* UIManager = GetUIManagerSubsystem(WorldContextObject);
	if (!UIManager)
	{
		return nullptr;
	}
	return UIManager->GetPrimaryGameLayout();
}

void UWxUILibrary::DeactivateOwningActivatable(UWidget* StartingWidget)
{
	if (!StartingWidget)
	{
		return;
	}

	for (UUserWidget* Outer = StartingWidget->GetTypedOuter<UUserWidget>(); Outer; Outer = Outer->GetTypedOuter<UUserWidget>())
	{
		if (UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(Outer))
		{
			Activatable->DeactivateWidget();
			return;
		}
	}
}

void UWxUILibrary::DeactivateWidgetsInLayer(const UObject* WorldContextObject, FGameplayTag LayerTag)
{
	UWxPrimaryGameLayout* Layout = GetPrimaryGameLayout(WorldContextObject);
	if (!Layout)
	{
		return;
	}

	UCommonActivatableWidgetStack* Stack = Layout->GetLayerWidgetStack(LayerTag);
	if (!Stack)
	{
		return;
	}

	Stack->ClearWidgets();
}
