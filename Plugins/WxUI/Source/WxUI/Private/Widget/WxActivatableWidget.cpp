// Copyright Woogle. All Rights Reserved.

#include "Widget/WxActivatableWidget.h"

TOptional<FUIInputConfig> UWxActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case ECommonInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, MouseCaptureMode);
	case ECommonInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, MouseCaptureMode);
	default:
		return TOptional<FUIInputConfig>();
	}
}
