// Copyright Woogle. All Rights Reserved.

#include "Component/WxPromptWidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/WxPromptWidgetInterface.h"

UWxPromptWidgetComponent::UWxPromptWidgetComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);
}

void UWxPromptWidgetComponent::InitWidget()
{
	Super::InitWidget();

	ApplyPromptTextToWidget();
}

void UWxPromptWidgetComponent::SetPromptText(const FText& InText)
{
	PromptText = InText;
	ApplyPromptTextToWidget();
}

void UWxPromptWidgetComponent::ApplyPromptTextToWidget()
{
	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	if (UserWidget->Implements<UWxPromptWidgetInterface>())
	{
		IWxPromptWidgetInterface::Execute_SetPromptText(UserWidget, PromptText);
	}
}
