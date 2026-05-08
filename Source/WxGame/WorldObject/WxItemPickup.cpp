// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxItemPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Controller/WxPlayerController.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Interaction/WxInteractionComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "WxCollisionChannels.h"

DEFINE_LOG_CATEGORY_STATIC(LogWxItemPickup, Log, All);

AWxItemPickup::AWxItemPickup()
{
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(WxCollision::WxAttack, ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(MeshComponent);
}

void AWxItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// ItemDef 는 스폰 직후 설정되고 이후 변하지 않으므로 초기 1회만 복제.
	DOREPLIFETIME_CONDITION(AWxItemPickup, ItemDef, COND_InitialOnly);
}

void AWxItemPickup::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxItemPickup::HandleInteracted);
}

void AWxItemPickup::SetItemDef(UWxItemDefinition* InItemDef)
{
	ItemDef = InItemDef;
	UpdateInteractionText();
	ApplyPickupVisual();
}

void AWxItemPickup::OnRep_ItemDef()
{
	UpdateInteractionText();
	ApplyPickupVisual();
}

void AWxItemPickup::UpdateInteractionText()
{
	if (!ItemDef)
	{
		return;
	}

	const FText FormattedText = FText::Format(NSLOCTEXT("WxItemPickup", "InteractionFormat", "[F] {0}"), ItemDef->DisplayName);
	InteractionComponent->SetInteractionText(FormattedText);
}

void AWxItemPickup::ApplyPickupVisual()
{
	if (!ItemDef)
	{
		return;
	}

	const UWxItemFragment_PickupVisual* Visual = ItemDef->FindFragmentByClass<UWxItemFragment_PickupVisual>();
	if (!Visual)
	{
		return;
	}

	if (UStaticMesh* MeshAsset = Visual->Mesh.LoadSynchronous())
	{
		MeshComponent->SetStaticMesh(MeshAsset);
	}

	if (UNiagaraSystem* NiagaraAsset = Visual->NiagaraSystem.LoadSynchronous())
	{
		NiagaraComponent->SetAsset(NiagaraAsset);
		NiagaraComponent->Activate(true);
	}
	else
	{
		NiagaraComponent->Deactivate();
	}
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
		if (AWxPlayerController* PC = Cast<AWxPlayerController>(InteractorPawn->GetController()))
		{
			Inventory = PC->GetInventoryManager();
		}
	}

	if (!Inventory)
	{
		Inventory = InteractingActor->FindComponentByClass<UWxInventoryManagerComponent>();
	}

	if (!Inventory)
	{
		UE_LOG(LogWxItemPickup, Warning, TEXT("Interactor %s has no UWxInventoryManagerComponent"), *InteractingActor->GetName());
		return;
	}

	UWxItemInstance* AddedInstance = Inventory->AddItemDefinition(ItemDef);
	const int32 TotalOwned = Inventory->GetTotalItemCountByDefinition(ItemDef);
	UE_LOG(LogWxItemPickup, Log, TEXT("Picked up %s (instance=%s, total=%d)"),
		*ItemDef->GetName(), *GetNameSafe(AddedInstance), TotalOwned);

	Destroy();
}
