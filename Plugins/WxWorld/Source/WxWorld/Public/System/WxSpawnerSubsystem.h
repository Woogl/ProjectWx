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

	/** 등록된 모든 Spawner에 대해 Respawn을 호출한다. */
	void RespawnAll();

private:
	TSet<TWeakObjectPtr<AWxSpawner>> RegisteredSpawners;
};
