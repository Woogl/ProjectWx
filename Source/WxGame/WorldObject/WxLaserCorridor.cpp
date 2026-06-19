// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxLaserCorridor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Interaction/WxInteractionComponent.h"
#include "Net/UnrealNetwork.h"
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

void AWxLaserCorridor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxLaserCorridor, State);
}

void AWxLaserCorridor::OnWxSaveRestored()
{
	// BeginPlay 이전 복원이면 곧 호출될 BeginPlay 가 RefreshLaserState 를 호출하므로 생략.
	// BeginPlay 이후 복원(스트리밍 인-스트림)이면 즉시 시각/인터랙션/타이머를 동기화한다.
	if (HasActorBegunPlay())
	{
		RefreshLaserState();
	}
}

void AWxLaserCorridor::BeginPlay()
{
	Super::BeginPlay();

	ConsoleInteraction->OnInteracted.AddDynamic(this, &AWxLaserCorridor::HandleConsoleInteracted);

	// Level Streaming Persistence + WxSave 복원: State 가 BeginPlay 직전에 직접 set 되었을 수 있으므로 명시 동기화.
	RefreshLaserState();
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

void AWxLaserCorridor::HandleConsoleInteracted(AActor* InstigatorActor)
{
	SetLaserCorridorState(EWxLaserCorridorState::Disabled);
}

void AWxLaserCorridor::OnRep_State()
{
	RefreshLaserState();
}

void AWxLaserCorridor::SetLaserCorridorState(EWxLaserCorridorState NewState)
{
	// State 쓰기는 권위 전용. 클라는 OnRep_State 로 동기화된다.
	if (!HasAuthority() || State == NewState)
	{
		return;
	}

	State = NewState;
	RefreshLaserState();
}

void AWxLaserCorridor::RefreshLaserState()
{
	if (State == EWxLaserCorridorState::Disabled)
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
