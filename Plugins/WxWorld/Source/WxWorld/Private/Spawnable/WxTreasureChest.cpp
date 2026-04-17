// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxTreasureChest.h"

#include "Component/WxInteractionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "Net/UnrealNetwork.h"

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

void AWxTreasureChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxTreasureChest, bIsOpened);
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
	if (!HasAuthority() || bIsOpened)
	{
		return;
	}

	bIsOpened = true;

	if (InteractionComponent)
	{
		InteractionComponent->SetInteractionEnabled(false);
	}

	if (ItemActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		const FVector SpawnLocation = GetActorTransform().TransformPosition(SpawnOffset);
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(ItemActorClass, SpawnLocation, GetActorRotation(), SpawnParams);

		if (Spawned)
		{
			UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(Spawned->GetRootComponent());
			if (PrimitiveRoot)
			{
				// 위쪽을 중심으로 원뿔 범위 내에서 랜덤 방향 샘플링
				const float ConeRad = FMath::DegreesToRadians(LaunchConeHalfAngle);
				FVector LaunchDir = FMath::VRandCone(FVector::UpVector, ConeRad);

				// 상자 뒤쪽 성분이 있으면 forward 축에 대해 반사해 앞/옆/위 반구로 제한
				const FVector ActorForward = GetActorForwardVector();
				const float ForwardDot = FVector::DotProduct(LaunchDir, ActorForward);
				if (ForwardDot < 0.f)
				{
					LaunchDir = (LaunchDir - 2.f * ForwardDot * ActorForward).GetSafeNormal();
				}

				PrimitiveRoot->SetPhysicsLinearVelocity(LaunchDir * LaunchSpeed);
			}
		}
	}
}

void AWxTreasureChest::OnRep_bIsOpened()
{
	if (bIsOpened && InteractionComponent)
	{
		InteractionComponent->SetInteractionEnabled(false);
	}
}

#if WITH_EDITOR
const UMeshComponent* AWxTreasureChest::GetEditorPreviewMeshComponent() const
{
	return MeshComponent;
}
#endif
