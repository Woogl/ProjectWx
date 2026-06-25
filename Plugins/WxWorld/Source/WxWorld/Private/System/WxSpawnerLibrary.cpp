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

	// Respawn() 이 루프 안에서 액터를 Destroy/Spawn 하므로, 순회 도중 월드 액터 배열 변경을 피하기 위해
	// 먼저 대상 스포너를 모은 뒤 순회를 끝내고 일괄 호출한다(collect-first).
	// Manual 스포너는 외부 개별 트리거 전용이라 일괄 리스폰에서 제외한다.
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
