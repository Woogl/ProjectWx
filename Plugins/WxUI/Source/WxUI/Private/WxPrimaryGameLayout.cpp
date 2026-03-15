// Copyright Woogle. All Rights Reserved.

#include "WxPrimaryGameLayout.h"
#include "WxGameplayTags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"


void UWxPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterLayer(WxGameplayTags::UI_Layer_Game, GameLayer);
	RegisterLayer(WxGameplayTags::UI_Layer_GameMenu, GameMenuLayer);
	RegisterLayer(WxGameplayTags::UI_Layer_Menu, MenuLayer);
	RegisterLayer(WxGameplayTags::UI_Layer_Modal, ModalLayer);
}

UCommonActivatableWidgetStack* UWxPrimaryGameLayout::GetLayerWidgetStack(FGameplayTag LayerTag) const
{
	const TObjectPtr<UCommonActivatableWidgetStack>* Found = LayerMap.Find(LayerTag);
	if (Found)
	{
		return *Found;
	}
	return nullptr;
}

UCommonActivatableWidget* UWxPrimaryGameLayout::PushWidgetToLayerStack(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	UCommonActivatableWidgetStack* Stack = GetLayerWidgetStack(LayerTag);
	if (!Stack)
	{
		return nullptr;
	}
	return Stack->AddWidget(WidgetClass);
}

UCommonActivatableWidget* UWxPrimaryGameLayout::PushWidgetInstanceToLayerStack(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance)
{
	if (!WidgetInstance)
	{
		return nullptr;
	}

	UCommonActivatableWidgetStack* Stack = GetLayerWidgetStack(LayerTag);
	if (!Stack)
	{
		return nullptr;
	}

	Stack->AddWidgetInstance(*WidgetInstance);
	return WidgetInstance;
}

void UWxPrimaryGameLayout::RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetStack* Stack)
{
	if (Stack)
	{
		LayerMap.Add(LayerTag, Stack);
	}
}
