// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionWidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Interaction/WxInteractionWidgetInterface.h"

UWxInteractionWidgetComponent::UWxInteractionWidgetComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);

	InteractionText = FText::FromString(TEXT("[F] Interact"));
}

void UWxInteractionWidgetComponent::InitWidget()
{
	Super::InitWidget();

	ApplyInteractionTextToWidget();
}

void UWxInteractionWidgetComponent::SetInteractionText(const FText& InText)
{
	InteractionText = InText;
	ApplyInteractionTextToWidget();
}

void UWxInteractionWidgetComponent::ApplyInteractionTextToWidget()
{
	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	if (UserWidget->Implements<UWxInteractionWidgetInterface>())
	{
		IWxInteractionWidgetInterface::Execute_SetInteractionText(UserWidget, InteractionText);
	}
}
