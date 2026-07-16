// Copyright Woogle. All Rights Reserved.

#include "Widget/WxActivatableWidget.h"

TOptional<FUIInputConfig> UWxActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case ECommonInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	case ECommonInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
	default:
		return TOptional<FUIInputConfig>();
	}
}

bool UWxActivatableWidget::ShouldPauseGame() const
{
	return bPauseGame;
}
