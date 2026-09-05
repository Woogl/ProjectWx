// Copyright Woogle. All Rights Reserved.

#include "WxRewardLibrary.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "GameFramework/Actor.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemPickup.h"
#include "Items/WxRewardTableRow.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxRewardLibrary, Log, All);

void UWxRewardLibrary::GrantReward(AActor* SourceActor, const FDataTableRowHandle& RewardRow, AActor* DirectGrantTarget, const FTransform& SpawnTransform, FVector LaunchVelocity)
{
	if (!SourceActor || !SourceActor->HasAuthority())
	{
		return;
	}

	// 보상 미지정 호출이 흔하므로 GetRow 의 로그 스팸을 피하기 위해 먼저 걸러낸다.
	if (RewardRow.IsNull())
	{
		return;
	}

	const FWxRewardTableRow* Row = RewardRow.GetRow<FWxRewardTableRow>(TEXT("UWxRewardLibrary::GrantReward"));
	if (!Row)
	{
		return;
	}

	TArray<FWxItemRewardEntry> ValidRewards;
	Row->GetValidRewards(ValidRewards);

	const APawn* TargetPawn = Cast<APawn>(DirectGrantTarget);
	const APlayerController* PlayerController = TargetPawn ? Cast<APlayerController>(TargetPawn->GetController()) : Cast<APlayerController>(DirectGrantTarget);
	UWxInventoryComponent* DirectGrantInventory = PlayerController ? PlayerController->FindComponentByClass<UWxInventoryComponent>() : nullptr;

	UWorld* World = SourceActor->GetWorld();

	for (const FWxItemRewardEntry& Reward : ValidRewards)
	{
		UWxItemDefinition* ItemDef = Reward.Item.LoadSynchronous();
		if (!ItemDef)
		{
			continue;
		}

		const UWxItemFragment_Pickup* PickupFragment = ItemDef->FindFragmentByClass<UWxItemFragment_Pickup>();
		if (!PickupFragment)
		{
			if (DirectGrantInventory)
			{
				DirectGrantInventory->AddItemDefinition(ItemDef, Reward.Quantity);
			}
			else
			{
				UE_LOG(LogWxRewardLibrary, Warning, TEXT("%s: reward item %s has no Pickup Fragment and no DirectGrantTarget inventory; skipped"), *GetNameSafe(SourceActor), *ItemDef->GetName());
			}
			continue;
		}

		UClass* ItemActorClass = PickupFragment->ItemActorClass.LoadSynchronous();
		if (!ItemActorClass)
		{
			UE_LOG(LogWxRewardLibrary, Warning, TEXT("%s: reward item %s has a Pickup Fragment but ItemActorClass is not set; skipped"), *GetNameSafe(SourceActor), *ItemDef->GetName());
			continue;
		}

		// Deferred 스폰: ItemDef/Quantity 가 COND_InitialOnly 라 FinishSpawning 전에 넣어야 클라에 전달된다.
		AWxItemPickup* SpawnedPickup = World->SpawnActorDeferred<AWxItemPickup>(ItemActorClass, SpawnTransform);
		if (!SpawnedPickup)
		{
			continue;
		}

		SpawnedPickup->SetItemDef(ItemDef, Reward.Quantity);
		SpawnedPickup->FinishSpawning(SpawnTransform);

		SpawnedPickup->LaunchInDirection(LaunchVelocity, LaunchVelocity.Size());
	}
}
