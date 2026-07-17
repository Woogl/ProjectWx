// Copyright Woogle. All Rights Reserved.


#include "Widget/WxButtonBase.h"

#include "CommonActionWidget.h"
#include "CommonTextBlock.h"

UWxButtonBase::UWxButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// TriggeringInputAction이 없는 버튼이 hover 시 DefaultClickAction(Enter) 아이콘을 띄우지 않게 한다.
	bShouldUseFallbackDefaultInputAction = false;
}

void UWxButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 엔진의 아이콘 채움/숨김(UpdateInputActionWidget)은 GameInstance 가드로 디자인 타임엔 안 도므로 직접 반영한다.
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

	UpdateButtonStyle();
	RefreshButtonText();
}

void UWxButtonBase::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();

	UpdateButtonStyle();
	RefreshButtonText();
}

void UWxButtonBase::SetButtonText(const FText& InText)
{
	bOverride_ButtonText = InText.IsEmpty();
	ButtonText = InText;
	RefreshButtonText();
}

void UWxButtonBase::RefreshButtonText()
{
	// 호버 클릭 폴백을 생성자에서 껐으므로, 액션이 없으면 GetDisplayText가 비어 엉뚱한 텍스트가 새지 않는다.
	if (bOverride_ButtonText || ButtonText.IsEmpty())
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

void UWxButtonBase::OnInputMethodChanged(ECommonInputType CurrentInputType)
{
	Super::OnInputMethodChanged(CurrentInputType);

	UpdateButtonStyle();
}
