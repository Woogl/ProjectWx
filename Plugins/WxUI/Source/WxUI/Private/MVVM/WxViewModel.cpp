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

bool UWxViewModel::IsInitialized() const
{
	return bInitialized;
}

void UWxViewModel::SetInitialized(bool bInValue)
{
	if (bInitialized == bInValue)
	{
		return;
	}

	bInitialized = bInValue;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsInitialized);
}
