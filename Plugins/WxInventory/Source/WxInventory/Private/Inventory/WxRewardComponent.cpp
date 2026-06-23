// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxRewardComponent.h"

#include "Components/ArrowComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemPickup.h"
#include "Items/WxRewardTableRow.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxRewardComponent, Log, All);

#if WITH_EDITORONLY_DATA
void UWxRewardComponent::OnRegister()
{
	Super::OnRegister();

	// 에디터 전용 시각화 컴포넌트는 에셋에 직렬화하지 않고 등록 시점에 동적 생성한다. (UCameraComponent 의 프리뷰 메시와 동일한 패턴)
	if (!ArrowComponent && GetOwner() && !IsRunningCommandlet())
	{
		ArrowComponent = NewObject<UArrowComponent>(GetOwner(), NAME_None, RF_Transactional | RF_TextExportTransient);
		ArrowComponent->SetupAttachment(this);
		// 화살표 메시는 +X 를 향하므로 발사 축인 업 벡터(+Z) 로 세운다.
		ArrowComponent->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		ArrowComponent->SetIsVisualizationComponent(true);
		ArrowComponent->CreationMethod = CreationMethod;
		ArrowComponent->RegisterComponentWithWorld(GetWorld());
	}
}
#endif

void UWxRewardComponent::DropRewards(AActor* DirectGrantTarget)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// 보상 미지정 오너가 흔하므로 GetRow 의 로그 스팸을 피하기 위해 먼저 걸러낸다.
	if (RewardRow.IsNull())
	{
		return;
	}

	const FWxRewardTableRow* Row = RewardRow.GetRow<FWxRewardTableRow>(TEXT("UWxRewardComponent::DropRewards"));
	if (!Row)
	{
		return;
	}

	TArray<FWxItemRewardEntry> ValidRewards;
	Row->GetValidRewards(ValidRewards);

	// Pickup Fragment 가 없는 보상은 월드에 띄울 외형이 없으므로 이 인벤토리에 즉시 지급한다.
	UWxInventoryManagerComponent* DirectGrantInventory = UWxInventoryManagerComponent::FindInventory(DirectGrantTarget);

	const FTransform SpawnTransform(GetComponentRotation(), GetComponentLocation());
	const float ConeRad = FMath::DegreesToRadians(LaunchConeHalfAngle);

	// 유효한 보상 항목마다 처리: Pickup Fragment 가 있으면 픽업을 스폰해 업 벡터 원뿔로 흩뿌리고, 없으면 인벤토리에 직접 지급한다.
	for (const FWxItemRewardEntry& Reward : ValidRewards)
	{
		// 보상 아이템 정의는 SoftObjectPtr 로 지연 로드된다. 실제 지급 시점인 지금 동기 로드한다.
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
				UE_LOG(LogWxRewardComponent, Warning, TEXT("%s: reward item %s has no Pickup Fragment and no DirectGrantTarget inventory; skipped"), *GetNameSafe(GetOwner()), *ItemDef->GetName());
			}
			continue;
		}

		// 픽업 액터 클래스는 SoftClassPtr 로 지연 로드된다. 실제 스폰 시점인 지금 동기 로드한다.
		UClass* ItemActorClass = PickupFragment->ItemActorClass.LoadSynchronous();
		if (!ItemActorClass)
		{
			UE_LOG(LogWxRewardComponent, Warning, TEXT("%s: reward item %s has a Pickup Fragment but ItemActorClass is not set; skipped"), *GetNameSafe(GetOwner()), *ItemDef->GetName());
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

		// 업 벡터를 중심으로 원뿔 범위 내에서 랜덤 방향으로 발사해 흩뿌린다.
		const FVector LaunchDir = FMath::VRandCone(GetUpVector(), ConeRad);
		SpawnedPickup->LaunchInDirection(LaunchDir, LaunchSpeed);
	}
}
