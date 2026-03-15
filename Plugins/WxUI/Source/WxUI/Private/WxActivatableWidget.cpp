// Copyright Woogle. All Rights Reserved.

#include "WxActivatableWidget.h"

void UWxActivatableWidget::SetViewModel(UWxViewModel* InViewModel)
{
	ViewModel = InViewModel;
}

UWxViewModel* UWxActivatableWidget::GetViewModel() const
{
	return ViewModel;
}
