// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"

#include "Cheat/WxCheatManager.h"
#include "Component/WxHUDComponent.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "Inventory/WxInventoryComponent.h"
#include "WxDialogueSessionComponent.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CheatClass = UWxCheatManager::StaticClass();

	InventoryComponent = CreateDefaultSubobject<UWxInventoryComponent>(TEXT("InventoryComponent"));
	InteractionScannerComponent = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScannerComponent"));
	DialogueSessionComponent = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSessionComponent"));
	HUDComponent = CreateDefaultSubobject<UWxHUDComponent>(TEXT("HUDComponent"));
}
