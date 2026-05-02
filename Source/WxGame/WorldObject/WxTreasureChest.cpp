// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxTreasureChest.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionWidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "WorldObject/WxItemPickup.h"

AWxTreasureChest::AWxTreasureChest()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionWidget->SetupAttachment(MeshComponent);
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxTreasureChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxTreasureChest, bIsOpened);
}

void AWxTreasureChest::BeginPlay()
{
	Super::BeginPlay();

	OnInteracted.AddDynamic(this, &AWxTreasureChest::HandleInteracted);
}

void AWxTreasureChest::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority() || bIsOpened)
	{
		return;
	}

	bIsOpened = true;

	SetInteractionEnabled(false);

	if (!ItemActorClass || !ItemDefinition)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 상자 메시의 바운딩 박스 상단 중앙에서 스폰 — 메시 크기에 무관하게 자연스러운 위치 보장.
	const FBoxSphereBounds Bounds = MeshComponent->Bounds;
	const FVector SpawnLocation(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z);
	AWxItemPickup* SpawnedPickup = GetWorld()->SpawnActor<AWxItemPickup>(ItemActorClass, SpawnLocation, GetActorRotation(), SpawnParams);
	if (!SpawnedPickup)
	{
		return;
	}

	SpawnedPickup->Initialize(ItemDefinition);

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

	SpawnedPickup->LaunchInDirection(LaunchDir, LaunchSpeed);
}

void AWxTreasureChest::OnRep_bIsOpened()
{
	if (bIsOpened)
	{
		SetInteractionEnabled(false);
	}
}
