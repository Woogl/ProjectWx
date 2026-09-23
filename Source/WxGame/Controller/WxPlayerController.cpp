// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"

#include "Character/WxCharacterBase.h"
#include "Cheat/WxCheatManager.h"
#include "Component/WxNameplateManagerComponent.h"
#include "Component/WxPlayerLayoutComponent.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "Inventory/WxInventoryComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxDialogueSessionComponent.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CheatClass = UWxCheatManager::StaticClass();

	InventoryComponent = CreateDefaultSubobject<UWxInventoryComponent>(TEXT("InventoryComponent"));
	InteractionScannerComponent = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScannerComponent"));
	DialogueSessionComponent = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSessionComponent"));
	PlayerLayoutComponent = CreateDefaultSubobject<UWxPlayerLayoutComponent>(TEXT("PlayerLayoutComponent"));
	NameplateManagerComponent = CreateDefaultSubobject<UWxNameplateManagerComponent>(TEXT("NameplateManagerComponent"));
}

void AWxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 락온은 WxCombat이 소유하므로 NameplateManager에는 여기서 잇는다.
	NameplateManagerComponent->LockOnTargetQuery.BindUObject(this, &AWxPlayerController::GetLockOnTarget);
}

USceneComponent* AWxPlayerController::GetLockOnTarget() const
{
	const AWxCharacterBase* PossessedCharacter = GetPawn<AWxCharacterBase>();
	return PossessedCharacter ? PossessedCharacter->GetLockOnComponent()->GetLockOnTarget() : nullptr;
}
