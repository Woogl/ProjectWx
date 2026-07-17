// Copyright Woogle. All Rights Reserved.

#include "System/WxPrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "WxGameplayTags.h"

UWxPrimaryGameLayout::UWxPrimaryGameLayout()
{
	LayerTags = {
		WxGameplayTags::UI_Layer_Game,
		WxGameplayTags::UI_Layer_GameMenu,
		WxGameplayTags::UI_Layer_Menu,
		WxGameplayTags::UI_Layer_Modal,
	};
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

void UWxPrimaryGameLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LayerMap.Reset();
	if (!LayerContainer || !WidgetTree)
	{
		return;
	}

	for (const FGameplayTag& LayerTag : LayerTags)
	{
		if (!LayerTag.IsValid())
		{
			continue;
		}

		UCommonActivatableWidgetStack* Stack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>();
		if (!Stack)
		{
			continue;
		}

		// 새 스택 CDO 기본 TransitionDuration(0.4s)을 0으로 되돌려 즉시전환을 유지한다.
		Stack->SetTransitionDuration(0.f);

		if (UOverlaySlot* LayerSlot = LayerContainer->AddChildToOverlay(Stack))
		{
			// Overlay 슬롯 기본 정렬은 Left/Top이라 전체 화면을 채우도록 명시한다.
			LayerSlot->SetHorizontalAlignment(HAlign_Fill);
			LayerSlot->SetVerticalAlignment(VAlign_Fill);
		}

		LayerMap.Add(LayerTag, Stack);
	}
}
