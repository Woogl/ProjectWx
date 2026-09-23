// Copyright Woogle. All Rights Reserved.

#include "WxUILibrary.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxGamePopup.h"
#include "CommonActivatableWidget.h"
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

void UWxUILibrary::ShowConfirmationPopup(const UObject* WorldContextObject, EWxPopupButtonLayout Buttons, FText Header, FText Body, const FWxPopupResultDynamicDelegate& OnResult)
{
	UWxUIManagerSubsystem* UIManager = GetUIManagerSubsystem(WorldContextObject);
	if (!UIManager)
	{
		OnResult.ExecuteIfBound(EWxPopupResult::Killed);
		return;
	}

	UWxGamePopupDescriptor* Descriptor = nullptr;
	switch (Buttons)
	{
	case EWxPopupButtonLayout::Ok:          Descriptor = UWxGamePopupDescriptor::CreateConfirmationOk(Header, Body); break;
	case EWxPopupButtonLayout::OkCancel:    Descriptor = UWxGamePopupDescriptor::CreateConfirmationOkCancel(Header, Body); break;
	case EWxPopupButtonLayout::YesNo:       Descriptor = UWxGamePopupDescriptor::CreateConfirmationYesNo(Header, Body); break;
	case EWxPopupButtonLayout::YesNoCancel: Descriptor = UWxGamePopupDescriptor::CreateConfirmationYesNoCancel(Header, Body); break;
	}

	FWxPopupResultDelegate ResultCallback;
	if (OnResult.IsBound())
	{
		ResultCallback = FWxPopupResultDelegate::CreateWeakLambda(OnResult.GetUObject(), [OnResult](EWxPopupResult Result)
		{
			OnResult.ExecuteIfBound(Result);
		});
	}

	UIManager->ShowConfirmation(Descriptor, ResultCallback);
}
