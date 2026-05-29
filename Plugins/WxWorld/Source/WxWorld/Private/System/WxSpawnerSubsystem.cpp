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
				Spawner->MarkKilled();
				return;
			}
		}
	}
}

void UWxSpawnerSubsystem::RespawnAutoSpawners()
{
	for (const TWeakObjectPtr<AWxSpawner>& Weak : RegisteredSpawners)
	{
		if (AWxSpawner* Spawner = Weak.Get())
		{
			// Manual 스포너는 외부 개별 트리거(SpawnConsole 등) 로만 스폰되며 일괄 리스폰 대상에서 제외.
			if (Spawner->GetSpawnMode() == EWxSpawnerMode::Manual)
			{
				continue;
			}

			Spawner->Respawn();
		}
	}
}
