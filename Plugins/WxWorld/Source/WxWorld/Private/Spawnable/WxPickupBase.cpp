// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxPickupBase.h"

#include "Component/WxInteractionComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxPickup, Log, All);

AWxPickupBase::AWxPickupBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionWidget = CreateDefaultSubobject<UWxInteractionWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(SceneRoot);
	InteractionComponent->InteractionWidget = InteractionWidget;

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void AWxPickupBase::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionComponent)
	{
		InteractionComponent->OnInteracted.AddDynamic(this, &AWxPickupBase::HandleInteracted);
	}
}

void AWxPickupBase::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || !InteractingActor)
	{
		return;
	}

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
		UE_LOG(LogWxPickup, Warning, TEXT("Interactor %s has no UWxInventoryManagerComponent"), *InteractingActor->GetName());
		Destroy();
		return;
	}

	const FWxAddItemResult Result = Inventory->AddItem(ItemDef, Count);
	const int32 TotalOwned = Inventory->GetItemCountByDef(ItemDef);
	UE_LOG(LogWxPickup, Log, TEXT("Picked up %s x%d (added=%d, total=%d)"), *ItemDef->GetName(), Count, Result.AmountAdded, TotalOwned);

	Destroy();
}

#if WITH_EDITOR
const UMeshComponent* AWxPickupBase::GetEditorPreviewMeshComponent() const
{
	return MeshComponent;
}
#endif
