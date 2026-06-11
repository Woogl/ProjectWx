// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxTreasureChest.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxRewardTableRow.h"
#include "WorldObject/WxItemPickup.h"

AWxTreasureChest::AWxTreasureChest()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxTreasureChest::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxTreasureChest::HandleInteracted);

	ApplyState();
}

void AWxTreasureChest::ApplyState()
{
	if (bTriggered)
	{
		InteractionComponent->SetInteractionEnabled(false);
	}
}

void AWxTreasureChest::HandleInteracted(AActor* InstigatorActor)
{
	if (!HasAuthority() || bTriggered)
	{
		return;
	}

	MarkTriggered();

	const FWxRewardTableRow* Row = RewardRow.GetRow<FWxRewardTableRow>(TEXT("AWxTreasureChest::HandleInteracted"));
	if (!Row)
	{
		return;
	}

	TArray<FWxItemRewardEntry> ValidRewards;
	Row->GetValidRewards(ValidRewards);

	// Pickup Fragment 가 없는 보상은 월드에 띄울 외형이 없으므로 상호작용자 인벤토리에 즉시 지급한다.
	UWxInventoryManagerComponent* InstigatorInventory = UWxInventoryManagerComponent::FindInventory(InstigatorActor);

	// 상자 메시의 바운딩 박스 상단 중앙에서 스폰 — 메시 크기에 무관하게 자연스러운 위치 보장.
	const FBoxSphereBounds Bounds = MeshComponent->Bounds;
	const FVector SpawnLocation(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z);
	const FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

	const float ConeRad = FMath::DegreesToRadians(LaunchConeHalfAngle);
	const FVector ActorForward = GetActorForwardVector();

	// 유효한 보상 항목마다 처리: Pickup Fragment 가 있으면 픽업을 스폰해 상자 위쪽 반구로 흩뿌리고, 없으면 인벤토리에 직접 지급한다.
	for (const FWxItemRewardEntry& Reward : ValidRewards)
	{
		// 보상 아이템 정의는 SoftObjectPtr 로 지연 로드된다. 실제 지급 시점인 지금 동기 로드한다.
		UWxItemDefinition* ItemDef = Reward.Item.LoadSynchronous();
		if (!ItemDef)
		{
			continue;
		}

		// Pickup Fragment 가 없는 아이템(예: 재화)은 외형이 없으므로 월드 픽업 없이 인벤토리에 곧장 지급한다.
		if (!ItemDef->FindFragmentByClass<UWxItemFragment_Pickup>())
		{
			if (InstigatorInventory)
			{
				InstigatorInventory->AddItemDefinition(ItemDef, Reward.Quantity);
			}
			continue;
		}

		if (!ItemActorClass)
		{
			continue;
		}

		// Deferred 스폰: BeginPlay 전에 ItemDef 를 주입해야 픽업의 인터랙션 텍스트가 BeginPlay 단독으로 갱신된다.
		AWxItemPickup* SpawnedPickup = GetWorld()->SpawnActorDeferred<AWxItemPickup>(ItemActorClass, SpawnTransform);
		if (!SpawnedPickup)
		{
			continue;
		}

		SpawnedPickup->SetItemDef(ItemDef, Reward.Quantity);
		SpawnedPickup->FinishSpawning(SpawnTransform);

		// 위쪽을 중심으로 원뿔 범위 내에서 랜덤 방향 샘플링
		FVector LaunchDir = FMath::VRandCone(FVector::UpVector, ConeRad);

		// 상자 뒤쪽 성분이 있으면 forward 축에 대해 반사해 앞/옆/위 반구로 제한
		const float ForwardDot = FVector::DotProduct(LaunchDir, ActorForward);
		if (ForwardDot < 0.f)
		{
			LaunchDir = (LaunchDir - 2.f * ForwardDot * ActorForward).GetSafeNormal();
		}

		SpawnedPickup->LaunchInDirection(LaunchDir, LaunchSpeed);
	}
}
