// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxLaserCorridor.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WxEffectZone.h"

AWxLaserCorridor::AWxLaserCorridor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CorridorVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CorridorVolume"));
	SetRootComponent(CorridorVolume);
	CorridorVolume->SetBoxExtent(FVector(1000.f, 250.f, 250.f));
	CorridorVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWxLaserCorridor::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !LaserZoneClass)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AWxLaserCorridor::HandleSpawnTimer, SpawnInterval, true, 0.f);
}

void AWxLaserCorridor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AWxLaserCorridor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || ActiveLasers.Num() == 0)
	{
		return;
	}

	const FVector Step = GetActorForwardVector() * MoveSpeed * DeltaSeconds;

	for (int32 Index = ActiveLasers.Num() - 1; Index >= 0; --Index)
	{
		AWxEffectZone* Zone = ActiveLasers[Index].Get();
		if (!Zone)
		{
			ActiveLasers.RemoveAtSwap(Index);
			continue;
		}

		Zone->SetActorLocation(Zone->GetActorLocation() + Step, false);
	}
}

void AWxLaserCorridor::HandleSpawnTimer()
{
	UWorld* World = GetWorld();
	if (!World || !LaserZoneClass || MoveSpeed <= 0.f)
	{
		return;
	}

	const FVector Forward = GetActorForwardVector();
	const FVector Extent = CorridorVolume->GetScaledBoxExtent();
	const FVector StartLocation = GetActorLocation() - Forward * Extent.X;

	const float Lifetime = (Extent.X * 2.f) / MoveSpeed;

	const FTransform SpawnTransform(GetActorRotation(), StartLocation);
	AWxEffectZone* Zone = World->SpawnActorDeferred<AWxEffectZone>(LaserZoneClass, SpawnTransform, this, GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Zone)
	{
		return;
	}
	Zone->SetLifeSpan(Lifetime);
	Zone->FinishSpawning(SpawnTransform);

	ActiveLasers.Add(Zone);
}
