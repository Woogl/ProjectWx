// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxSpawnerSubsystem.generated.h"

class AWxSpawner;

/**
 * 월드 내 AWxSpawner들을 레지스트리로 관리하는 서브시스템.
 * TActorIterator 전수 순회를 피하기 위해 Spawner가 BeginPlay/EndPlay 시점에 자신을 등록/해제한다.
 */
UCLASS()
class WXWORLD_API UWxSpawnerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterSpawner(AWxSpawner* Spawner);
	void UnregisterSpawner(AWxSpawner* Spawner);

	/** 해당 Spawner를 처치 상태로 기록한다. 이후 재스트리밍 시 해당 Spawner는 스폰을 스킵한다. */
	void MarkSpawnerKilled(const AWxSpawner* Spawner);

	/** Spawner 가 보유한 액터를 통해 역조회 후 처치 기록을 남긴다. 캐릭터 사망 시 호출. */
	void MarkSpawnableKilled(const AActor* SpawnedActor);

	/** 해당 Spawner가 처치 상태인지 조회한다. */
	bool IsSpawnerKilled(const AWxSpawner* Spawner) const;

	/** 처치 기록을 모두 초기화하고 로드된 모든 Spawner에 대해 Respawn을 호출한다. */
	void RespawnAll();

private:
	TSet<TWeakObjectPtr<AWxSpawner>> RegisteredSpawners;

	/** 부활 대상 처치 기록. RespawnAll 시 일괄 초기화된다. */
	TSet<FGuid> KilledSpawnerIds;

	/** 영구 사망(bEnableRespawn=false) Spawner의 처치 기록. RespawnAll에서도 보존된다. */
	TSet<FGuid> PermanentlyKilledSpawnerIds;
};
