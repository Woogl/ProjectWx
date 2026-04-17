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
	if (SpawnerId.IsValid())
	{
		KilledSpawnerIds.Add(SpawnerId);
	}
}

bool UWxSpawnerSubsystem::IsSpawnerKilled(const AWxSpawner* Spawner) const
{
	if (!Spawner)
	{
		return false;
	}

	return KilledSpawnerIds.Contains(Spawner->GetActorGuid());
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
