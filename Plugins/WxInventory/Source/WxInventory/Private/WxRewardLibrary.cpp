// Copyright Woogle. All Rights Reserved.

#include "WxRewardLibrary.h"

#include "GameFramework/Actor.h"
#include "Inventory/WxInventoryManagerComponent.h"
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

	// Pickup Fragment 가 없는 보상은 월드에 띄울 외형이 없으므로 이 인벤토리에 즉시 지급한다.
	UWxInventoryManagerComponent* DirectGrantInventory = UWxInventoryManagerComponent::FindInventory(DirectGrantTarget);

	UWorld* World = SourceActor->GetWorld();

	// 유효한 보상 항목마다 처리: Pickup Fragment 가 있으면 픽업을 스폰해 LaunchVelocity 방향·크기로 발사하고, 없으면 인벤토리에 직접 지급한다.
	for (const FWxItemRewardEntry& Reward : ValidRewards)
	{
		// 보상 아이템 정의는 SoftObjectPtr 로 지연 로드된다.
		// 실제 지급 시점인 지금 동기 로드한다.
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

		// 픽업 액터 클래스는 SoftClassPtr 로 지연 로드된다.
		// 실제 스폰 시점인 지금 동기 로드한다.
		UClass* ItemActorClass = PickupFragment->ItemActorClass.LoadSynchronous();
		if (!ItemActorClass)
		{
			UE_LOG(LogWxRewardLibrary, Warning, TEXT("%s: reward item %s has a Pickup Fragment but ItemActorClass is not set; skipped"), *GetNameSafe(SourceActor), *ItemDef->GetName());
			continue;
		}

		// Deferred 스폰: BeginPlay 전에 ItemDef 를 주입해야 픽업의 인터랙션 텍스트가 BeginPlay 단독으로 갱신된다.
		AWxItemPickup* SpawnedPickup = World->SpawnActorDeferred<AWxItemPickup>(ItemActorClass, SpawnTransform);
		if (!SpawnedPickup)
		{
			continue;
		}

		SpawnedPickup->SetItemDef(ItemDef, Reward.Quantity);
		SpawnedPickup->FinishSpawning(SpawnTransform);

		// 트랜스폼과 무관하게 LaunchVelocity 가 가리키는 방향·크기로 발사한다(LaunchInDirection 이 내부에서 정규화 후 곱하므로 결과 선속도 = LaunchVelocity).
		SpawnedPickup->LaunchInDirection(LaunchVelocity, LaunchVelocity.Size());
	}
}
