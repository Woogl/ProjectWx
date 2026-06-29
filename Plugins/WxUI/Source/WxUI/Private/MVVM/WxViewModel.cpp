// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel.h"

void UWxViewModel::BeginDestroy()
{
	Deinitialize();
	Super::BeginDestroy();
}

void UWxViewModel::Deinitialize()
{
}
