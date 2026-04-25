// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxPickupBase.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "GameFramework/RotatingMovementComponent.h"

AWxPickupBase::AWxPickupBase()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionWidget->SetupAttachment(SceneRoot);
	InteractionComponent->SetupAttachment(SceneRoot);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void AWxPickupBase::BeginPlay()
{
	Super::BeginPlay();

	OnInteracted.AddDynamic(this, &AWxPickupBase::HandleInteracted);
}

void AWxPickupBase::OnPickedUp(AActor* /*InteractingActor*/)
{
}

void AWxPickupBase::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || !InteractingActor)
	{
		return;
	}

	OnPickedUp(InteractingActor);
}

#if WITH_EDITOR
const UMeshComponent* AWxPickupBase::GetEditorPreviewMeshComponent() const
{
	return MeshComponent;
}
#endif
