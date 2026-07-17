// Copyright Woogle. All Rights Reserved.


#include "Widget/WxButtonBase.h"

#include "CommonActionWidget.h"
#include "CommonTextBlock.h"

UWxButtonBase::UWxButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShouldUseFallbackDefaultInputAction = false;
}

void UWxButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime() && InputActionWidget)
	{
		if (bHideInputAction || TriggeringInputAction.IsNull())
		{
			InputActionWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			InputActionWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			InputActionWidget->SetInputAction(TriggeringInputAction);
		}
	}

	RefreshButtonText();
}

void UWxButtonBase::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();

	RefreshButtonText();
}

void UWxButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;
	RefreshButtonText();
}

void UWxButtonBase::RefreshButtonText()
{
	if (ButtonText.IsEmpty())
	{
		if (InputActionWidget)
		{
			const FText ActionDisplayText = InputActionWidget->GetDisplayText();
			if (!ActionDisplayText.IsEmpty())
			{
				UpdateButtonText(ActionDisplayText);
				return;
			}
		}
	}

	UpdateButtonText(ButtonText);
}

void UWxButtonBase::UpdateButtonText(const FText& InText)
{
	if (TextBlock)
	{
		TextBlock->SetText(InText);
	}
}
