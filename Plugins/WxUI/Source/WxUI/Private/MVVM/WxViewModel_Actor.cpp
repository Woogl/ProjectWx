// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Actor.h"

void UWxViewModel_Actor::Initialize(const FText& InDisplayName)
{
	Deinitialize();

	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InDisplayName);

	SetInitialized(true);
}

void UWxViewModel_Actor::Deinitialize()
{
	DisplayName = FText::GetEmpty();

	SetInitialized(false);

	Super::Deinitialize();
}
