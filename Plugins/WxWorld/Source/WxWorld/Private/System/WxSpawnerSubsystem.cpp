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

void UWxSpawnerSubsystem::RespawnAll()
{
	for (const TWeakObjectPtr<AWxSpawner>& Weak : RegisteredSpawners)
	{
		if (AWxSpawner* Spawner = Weak.Get())
		{
			Spawner->Respawn();
		}
	}
}
