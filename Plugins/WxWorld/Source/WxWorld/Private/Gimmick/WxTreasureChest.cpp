// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxTreasureChest.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"

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
}
