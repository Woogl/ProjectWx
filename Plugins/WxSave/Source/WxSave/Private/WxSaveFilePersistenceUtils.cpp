// Copyright Woogle. All Rights Reserved.

#include "WxSaveFilePersistenceUtils.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "WxPersistenceGameSubsystem.h"

namespace
{
	UWxPersistenceGameSubsystem* GetSubsystem(const UObject* WorldContextObject)
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

		return GameInstance->GetSubsystem<UWxPersistenceGameSubsystem>();
	}
}

UWxPersistenceSaveGame* UWxSaveFilePersistenceUtils::StartNewSaveFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex, TSubclassOf<UWxPersistenceSaveGame> SpecificClass)
{
	if (UWxPersistenceGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->StartNewSaveFile(SlotName, UserIndex, SpecificClass);
	}

	return nullptr;
}

UWxPersistenceSaveGame* UWxSaveFilePersistenceUtils::LoadFromFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	if (UWxPersistenceGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->LoadFromFile(SlotName, UserIndex);
	}

	return nullptr;
}

UWxPersistenceSaveGame* UWxSaveFilePersistenceUtils::ReloadFromFile(const UObject* WorldContextObject, bool bStartTravel)
{
	if (UWxPersistenceGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->ReloadFromFile(bStartTravel);
	}

	return nullptr;
}

void UWxSaveFilePersistenceUtils::SaveToFile(const UObject* WorldContextObject)
{
	if (UWxPersistenceGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->SaveToFile();
	}
}

void UWxSaveFilePersistenceUtils::TravelFromSaveFile(const UObject* WorldContextObject)
{
	if (UWxPersistenceGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->TravelFromSaveFile();
	}
}

FString UWxSaveFilePersistenceUtils::GetDefaultSlotName()
{
	return WxPersistence::DefaultSlotName;
}
