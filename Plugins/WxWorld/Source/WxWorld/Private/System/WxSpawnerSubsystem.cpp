// Copyright Woogle. All Rights Reserved.

#include "System/WxSpawnerSubsystem.h"
#include "Spawnable/WxSpawner.h"

void UWxSpawnerSubsystem::RegisterSpawner(AWxSpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	RegisteredSpawners.Add(Spawner);
}

void UWxSpawnerSubsystem::UnregisterSpawner(AWxSpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	RegisteredSpawners.Remove(Spawner);
}

void UWxSpawnerSubsystem::MarkSpawnerKilled(const AWxSpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	const FGuid SpawnerId = Spawner->GetActorGuid();
	if (!SpawnerId.IsValid())
	{
		return;
	}

	if (Spawner->IsRespawnEnabled())
	{
		KilledSpawnerIds.Add(SpawnerId);
	}
	else
	{
		PermanentlyKilledSpawnerIds.Add(SpawnerId);
	}
}

void UWxSpawnerSubsystem::MarkSpawnableKilled(const AActor* SpawnedActor)
{
	if (!SpawnedActor)
	{
		return;
	}

	for (const TWeakObjectPtr<AWxSpawner>& Weak : RegisteredSpawners)
	{
		if (AWxSpawner* Spawner = Weak.Get())
		{
			if (Spawner->GetSpawnedActor() == SpawnedActor)
			{
				MarkSpawnerKilled(Spawner);
				return;
			}
		}
	}
}

bool UWxSpawnerSubsystem::IsSpawnerKilled(const AWxSpawner* Spawner) const
{
	if (!Spawner)
	{
		return false;
	}

	const FGuid SpawnerId = Spawner->GetActorGuid();
	return KilledSpawnerIds.Contains(SpawnerId) || PermanentlyKilledSpawnerIds.Contains(SpawnerId);
}

void UWxSpawnerSubsystem::RespawnAll()
{
	KilledSpawnerIds.Empty();

	for (const TWeakObjectPtr<AWxSpawner>& Weak : RegisteredSpawners)
	{
		if (AWxSpawner* Spawner = Weak.Get())
		{
			Spawner->Respawn();
		}
	}
}
