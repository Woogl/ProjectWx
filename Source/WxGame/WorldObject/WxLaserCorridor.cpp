// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxLaserCorridor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Interaction/WxInteractionComponent.h"
#include "TimerManager.h"
#include "WxEffectZone.h"

AWxLaserCorridor::AWxLaserCorridor()
{
	PrimaryActorTick.bCanEverTick = true;

	CorridorBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CorridorVolume"));
	CorridorBox->SetupAttachment(SceneRoot);
	CorridorBox->SetBoxExtent(FVector(1000.f, 250.f, 250.f));
	CorridorBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Console = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Console"));
	Console->SetupAttachment(SceneRoot);

	ConsoleInteraction = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("ConsoleInteraction"));
	ConsoleInteraction->SetupAttachment(Console);
}

void AWxLaserCorridor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxLaserCorridor::HandleConsoleInteracted);

	// Level Streaming Persistence + WxSave 복원: bTriggered 가 BeginPlay 직전에 직접 set 되었을 수 있으므로 명시 동기화.
	ApplyState();
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

	const FVector Step = CorridorBox->GetForwardVector() * MoveSpeed * DeltaSeconds;

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

void AWxLaserCorridor::ApplyState()
{
	if (bTriggered)
	{
		ConsoleInteraction->SetInteractionEnabled(false);

		if (HasAuthority())
		{
			GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

			for (TWeakObjectPtr<AWxEffectZone>& WeakZone : ActiveLasers)
			{
				if (AWxEffectZone* Zone = WeakZone.Get())
				{
					Zone->Destroy();
				}
			}
			ActiveLasers.Reset();
		}
	}
	else
	{
		ConsoleInteraction->SetInteractionEnabled(true);

		if (HasAuthority() && LaserZoneClass && !GetWorldTimerManager().IsTimerActive(SpawnTimerHandle))
		{
			GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AWxLaserCorridor::HandleSpawnTimer, SpawnInterval, true, 0.f);
		}
	}
}

void AWxLaserCorridor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	MarkTriggered();
}

void AWxLaserCorridor::HandleSpawnTimer()
{
	UWorld* World = GetWorld();
	if (!World || !LaserZoneClass || MoveSpeed <= 0.f)
	{
		return;
	}

	const FVector Forward = CorridorBox->GetForwardVector();
	const FVector Extent = CorridorBox->GetScaledBoxExtent();
	const FVector CorridorCenter = CorridorBox->GetComponentLocation();
	const FVector StartLocation = CorridorCenter - Forward * Extent.X;

	const float Lifetime = (Extent.X * 2.f) / MoveSpeed;

	// 벽의 BP 디폴트 YZ extent 를 기준으로, 통로의 scaled YZ extent 에 맞는 스케일을 계산해 SpawnTransform 에 반영.
	FVector SpawnScale(1.f, 1.f, 1.f);
	if (const AWxEffectZone* ZoneCDO = LaserZoneClass->GetDefaultObject<AWxEffectZone>())
	{
		if (const UBoxComponent* WallBoxCDO = ZoneCDO->FindComponentByClass<UBoxComponent>())
		{
			const FVector WallDefaultExtent = WallBoxCDO->GetUnscaledBoxExtent();
			if (WallDefaultExtent.Y > KINDA_SMALL_NUMBER && WallDefaultExtent.Z > KINDA_SMALL_NUMBER)
			{
				SpawnScale.Y = Extent.Y / WallDefaultExtent.Y;
				SpawnScale.Z = Extent.Z / WallDefaultExtent.Z;
			}
		}
	}

	const FTransform SpawnTransform(CorridorBox->GetComponentRotation(), StartLocation, SpawnScale);
	AWxEffectZone* Zone = World->SpawnActorDeferred<AWxEffectZone>(LaserZoneClass, SpawnTransform, this, GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Zone)
	{
		return;
	}
	Zone->SetLifeSpan(Lifetime);
	Zone->FinishSpawning(SpawnTransform);

	ActiveLasers.Add(Zone);
}
