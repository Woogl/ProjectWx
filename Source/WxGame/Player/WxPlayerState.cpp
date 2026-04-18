// Copyright Woogle. All Rights Reserved.

#include "Player/WxPlayerState.h"

#include "Inventory/WxInventoryManagerComponent.h"

AWxPlayerState::AWxPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InventoryManager = CreateDefaultSubobject<UWxInventoryManagerComponent>(TEXT("InventoryManager"));
}

UWxInventoryManagerComponent* AWxPlayerState::GetInventoryManager() const
{
	return InventoryManager;
}
