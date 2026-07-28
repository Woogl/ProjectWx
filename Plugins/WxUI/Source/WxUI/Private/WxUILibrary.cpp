// Copyright Woogle. All Rights Reserved.

#include "WxUILibrary.h"
#include "System/WxUIManagerSubsystem.h"
#include "System/WxPrimaryGameLayout.h"
#include "Widget/WxGamePopup.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** BP 다이나믹 결과 델리게이트를 서브시스템이 받는 네이티브 델리게이트로 브릿지한다. */
	FWxPopupResultDelegate MakeNativeResultDelegate(const FWxPopupResultDynamicDelegate& OnResult)
	{
		if (!OnResult.IsBound())
		{
			return FWxPopupResultDelegate();
		}

		// 바인딩 대상 UObject 수명에 묶어, 대상이 사라지면 호출되지 않도록 한다.
		return FWxPopupResultDelegate::CreateWeakLambda(OnResult.GetUObject(), [OnResult](EWxPopupResult Result)
		{
			OnResult.ExecuteIfBound(Result);
		});
	}
}

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

UCommonActivatableWidget* UWxUILibrary::PushSoftContentToLayer(const UObject* WorldContextObject, FGameplayTag LayerTag, TSoftClassPtr<UCommonActivatableWidget> WidgetClass)
{
	UWxUIManagerSubsystem* UIManager = GetUIManagerSubsystem(WorldContextObject);
	if (!UIManager)
	{
		return nullptr;
	}

	return UIManager->PushSoftContentToLayer(LayerTag, WidgetClass);
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

void UWxUILibrary::ShowConfirmationPopup(const UObject* WorldContextObject, EWxPopupButtonLayout Buttons, FText Header, FText Body, const FWxPopupResultDynamicDelegate& OnResult)
{
	UWxUIManagerSubsystem* UIManager = GetUIManagerSubsystem(WorldContextObject);
	if (!UIManager)
	{
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

	UIManager->ShowConfirmation(Descriptor, MakeNativeResultDelegate(OnResult));
}

void UWxUILibrary::ShowErrorPopup(const UObject* WorldContextObject, FText Header, FText Body, const FWxPopupResultDynamicDelegate& OnResult)
{
	UWxUIManagerSubsystem* UIManager = GetUIManagerSubsystem(WorldContextObject);
	if (!UIManager)
	{
		return;
	}

	UWxGamePopupDescriptor* Descriptor = UWxGamePopupDescriptor::CreateConfirmationOk(Header, Body);
	UIManager->ShowError(Descriptor, MakeNativeResultDelegate(OnResult));
}
