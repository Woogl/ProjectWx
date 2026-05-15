// Copyright Woogle. All Rights Reserved.

#include "System/WxSpawnerLibrary.h"

#include "Engine/World.h"
#include "System/WxSpawnerSubsystem.h"

void UWxSpawnerLibrary::RespawnAllSpawners(const UObject* WorldContextObject)
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

	if (UWxSpawnerSubsystem* Subsystem = World->GetSubsystem<UWxSpawnerSubsystem>())
	{
		Subsystem->RespawnAll();
	}
}
