// Copyright Woogle. All Rights Reserved.

#include "WxSaveGameLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "WxSaveGameSubsystem.h"

namespace
{
	UWxSaveGameSubsystem* GetSubsystem(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		const UWorld* World = WorldContextObject->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		const UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance)
		{
			return nullptr;
		}

		return GameInstance->GetSubsystem<UWxSaveGameSubsystem>();
	}
}

void UWxSaveGameLibrary::SaveSlot(const UObject* WorldContextObject, const FString& SlotName)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->SaveSlot(SlotName);
	}
}

bool UWxSaveGameLibrary::LoadSlot(const UObject* WorldContextObject, const FString& SlotName)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->LoadSlot(SlotName);
	}

	return false;
}

