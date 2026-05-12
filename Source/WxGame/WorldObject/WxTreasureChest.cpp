// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxTreasureChest.h"

#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "WorldObject/WxItemPickup.h"

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

	if (!ItemActorClass || !ItemDefinition)
	{
		return;
	}

	// 상자 메시의 바운딩 박스 상단 중앙에서 스폰 — 메시 크기에 무관하게 자연스러운 위치 보장.
	const FBoxSphereBounds Bounds = MeshComponent->Bounds;
	const FVector SpawnLocation(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z);
	const FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

	// Deferred 스폰: BeginPlay 전에 ItemDef 를 주입해야 픽업의 인터랙션 텍스트가 BeginPlay 단독으로 갱신된다.
	AWxItemPickup* SpawnedPickup = GetWorld()->SpawnActorDeferred<AWxItemPickup>(ItemActorClass, SpawnTransform);
	if (!SpawnedPickup)
	{
		return;
	}

	SpawnedPickup->SetItemDef(ItemDefinition);
	SpawnedPickup->FinishSpawning(SpawnTransform);

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
