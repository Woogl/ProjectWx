// Copyright Woogle. All Rights Reserved.

#include "WxPrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"
#include "WxGameplayTags.h"

void UWxPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LayerMap.Add(WxGameplayTags::UI_Layer_Game, GameLayer);
	LayerMap.Add(WxGameplayTags::UI_Layer_GameMenu, GameMenuLayer);
	LayerMap.Add(WxGameplayTags::UI_Layer_Menu, MenuLayer);
	LayerMap.Add(WxGameplayTags::UI_Layer_Modal, ModalLayer);
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
