// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxItemPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionWidgetComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxItemPickup, Log, All);

AWxItemPickup::AWxItemPickup()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// 기본 상태(idle): 충돌 비활성. 폰은 InteractionComponent(Sphere) 로 감지하므로 메시 충돌은 발사 시점에만 활성화한다.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	InteractionWidget->SetupAttachment(MeshComponent);
	InteractionComponent->SetupAttachment(MeshComponent);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(MeshComponent);
}

void AWxItemPickup::BeginPlay()
{
	Super::BeginPlay();

	OnInteracted.AddDynamic(this, &AWxItemPickup::HandleInteracted);
}

void AWxItemPickup::Initialize(UWxItemDefinition* InItemDef)
{
	ItemDef = InItemDef;
}

void AWxItemPickup::LaunchInDirection(const FVector& Direction, float Speed)
{
	if (!HasAuthority())
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetPhysicsLinearVelocity(Direction.GetSafeNormal() * Speed);
}

void AWxItemPickup::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || !InteractingActor)
	{
		return;
	}

	if (!ItemDef)
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
		UE_LOG(LogWxItemPickup, Warning, TEXT("Interactor %s has no UWxInventoryManagerComponent"), *InteractingActor->GetName());
		Destroy();
		return;
	}

	int32 GrantCount = 1;
	if (const FWxItemFragment_Currency* Currency = ItemDef->FindFragment<FWxItemFragment_Currency>())
	{
		GrantCount = Currency->Quantity;
	}

	const FWxAddItemResult Result = Inventory->AddItem(ItemDef, GrantCount);
	const int32 TotalOwned = Inventory->GetItemCountByDef(ItemDef);
	UE_LOG(LogWxItemPickup, Log, TEXT("Picked up %s x%d (added=%d, total=%d)"), *ItemDef->GetName(), GrantCount, Result.AmountAdded, TotalOwned);

	Destroy();
}
