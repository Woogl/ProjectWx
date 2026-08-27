// Copyright Woogle. All Rights Reserved.

#include "System/WxSpawnerLibrary.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Spawnable/WxSpawner.h"

void UWxSpawnerLibrary::TryRespawnAll(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	// Respawn() 이 액터를 Destroy/Spawn 하므로, 순회 도중 월드 액터 배열이 바뀌지 않도록 먼저 모은 뒤 일괄 호출한다.
	TArray<AWxSpawner*> Spawners;
	for (TActorIterator<AWxSpawner> It(World); It; ++It)
	{
		AWxSpawner* Spawner = *It;
		if (Spawner && Spawner->GetSpawnMode() != EWxSpawnerMode::Manual)
		{
			Spawners.Add(Spawner);
		}
	}

	for (AWxSpawner* Spawner : Spawners)
	{
		Spawner->Respawn();
	}
}

