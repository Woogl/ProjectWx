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
		// 스톡 경로는 GameInstance가 있어야 입력 액션을 피드하므로, 디자인타임엔 여기서 직접 피드한다.
		const bool bHasEnhancedAction = TriggeringEnhancedInputAction != nullptr;
		if (bHideInputAction || (TriggeringInputAction.IsNull() && !bHasEnhancedAction))
		{
			InputActionWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			InputActionWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (bHasEnhancedAction)
			{
				InputActionWidget->SetEnhancedInputAction(TriggeringEnhancedInputAction);
			}
			else
			{
				InputActionWidget->SetInputAction(TriggeringInputAction);
			}
		}
	}

	RefreshButtonText();
}

void UWxButtonBase::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();

	RefreshButtonText();
}

void UWxButtonBase::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	if (TextBlock)
	{
		TextBlock->SetStyle(GetCurrentTextStyleClass());
	}
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
