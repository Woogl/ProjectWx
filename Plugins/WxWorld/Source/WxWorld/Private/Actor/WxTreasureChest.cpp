// Copyright Woogle. All Rights Reserved.

#include "Actor/WxTreasureChest.h"

#include "Component/WxInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

AWxTreasureChest::AWxTreasureChest()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);

	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(MeshComponent);
	PromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidget->SetDrawAtDesiredSize(true);
	PromptWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PromptWidget->SetVisibility(false);
}

void AWxTreasureChest::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && InteractionComponent)
	{
		InteractionComponent->OnInteracted.AddDynamic(this, &AWxTreasureChest::HandleInteracted);
	}
}

void AWxTreasureChest::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || !InteractingActor)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("HandleInteracted!!! %s"), *InteractingActor->GetName());
}
