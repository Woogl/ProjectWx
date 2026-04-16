// Copyright Woogle. All Rights Reserved.

#include "Actor/WxTreasureChest.h"

#include "Component/WxInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AWxTreasureChest::AWxTreasureChest()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionWidget = CreateDefaultSubobject<UWxInteractionWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(MeshComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
	InteractionComponent->InteractionWidget = InteractionWidget;
}

void AWxTreasureChest::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionComponent)
	{
		InteractionComponent->OnInteracted.AddDynamic(this, &AWxTreasureChest::HandleInteracted);
	}
}

void AWxTreasureChest::HandleInteracted(AActor* InteractingActor)
{
	if (!NiagaraSystem || !MeshComponent)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		MeshComponent,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true);
}

#if WITH_EDITOR
UStreamableRenderAsset* AWxTreasureChest::GetEditorPreviewMesh() const
{
	return MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
}
#endif
