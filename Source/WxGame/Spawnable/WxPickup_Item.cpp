// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxPickup_Item.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxPickupItem, Log, All);

void AWxPickup_Item::OnPickedUp(AActor* InteractingActor)
{
	Super::OnPickedUp(InteractingActor);

	if (!ItemDef || Count <= 0)
	{
		Destroy();
		return;
	}

	UWxInventoryManagerComponent* Inventory = nullptr;
	if (const APawn* InteractorPawn = Cast<APawn>(InteractingActor))
	{
		if (APlayerState* PlayerState = InteractorPawn->GetPlayerState())
		{
			Inventory = PlayerState->FindComponentByClass<UWxInventoryManagerComponent>();
		}
	}

	if (!Inventory)
	{
		Inventory = InteractingActor->FindComponentByClass<UWxInventoryManagerComponent>();
	}

	if (!Inventory)
	{
		UE_LOG(LogWxPickupItem, Warning, TEXT("Interactor %s has no UWxInventoryManagerComponent"), *InteractingActor->GetName());
		Destroy();
		return;
	}

	const FWxAddItemResult Result = Inventory->AddItem(ItemDef, Count);
	const int32 TotalOwned = Inventory->GetItemCountByDef(ItemDef);
	UE_LOG(LogWxPickupItem, Log, TEXT("Picked up %s x%d (added=%d, total=%d)"), *ItemDef->GetName(), Count, Result.AmountAdded, TotalOwned);

	Destroy();
}
