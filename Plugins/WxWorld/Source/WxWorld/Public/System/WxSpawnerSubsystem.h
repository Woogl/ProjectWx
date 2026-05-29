// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxSpawnerSubsystem.generated.h"

class AWxSpawner;

/**
 * 월드 내 AWxSpawner들을 레지스트리로 관리하는 서브시스템.
 * TActorIterator 전수 순회를 피하기 위해 Spawner가 BeginPlay/EndPlay 시점에 자신을 등록/해제한다.
 *
 * 처치 상태는 각 AWxSpawner 자신이 bIsKilled (UPROPERTY SaveGame) 으로 보유한다.
 * 본 서브시스템은 상태를 직접 보존하지 않고, 등록된 Spawner 들에 위임/일괄 호출만 담당한다.
 */
UCLASS()
class WXWORLD_API UWxSpawnerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterSpawner(AWxSpawner* Spawner);
	void UnregisterSpawner(AWxSpawner* Spawner);

	/** Spawner 가 보유한 액터를 통해 역조회 후 해당 Spawner 를 처치 마킹. 캐릭터 사망 시 호출. */
	void MarkSpawnableKilled(const AActor* SpawnedActor);

	/** Auto 모드 Spawner 에 대해 Respawn 호출. Manual 모드는 개별 트리거 전용이라 제외. 영구 사망 (보스) 은 스킵. */
	void RespawnAutoSpawners();

private:
	TSet<TWeakObjectPtr<AWxSpawner>> RegisteredSpawners;
};
