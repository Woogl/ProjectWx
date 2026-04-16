// Copyright Woogle. All Rights Reserved.

#include "Actor/WxPickupBase.h"

#include "Component/WxInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "GameFramework/RotatingMovementComponent.h"

AWxPickupBase::AWxPickupBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(MeshComponent);

	InteractionWidget = CreateDefaultSubobject<UWxInteractionWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(MeshComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
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

	// TODO: 상호작용 시 아이템 획득 처리 구현 예정
	
	Destroy();
}

#if WITH_EDITOR
UStreamableRenderAsset* AWxPickupBase::GetEditorPreviewMesh() const
{
	return MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
}
#endif
