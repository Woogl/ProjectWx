// Copyright Woogle. All Rights Reserved.

#include "WxSaveLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "WxSaveGame.h"
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

FString UWxSaveLibrary::GetCurrentSaveSlotName(const UObject* WorldContextObject)
{
	if (const UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		if (const UWxSaveGame* SaveGame = Subsystem->GetSaveGame())
		{
			return SaveGame->SlotName;
		}
	}

	return FString();
}

UWxSaveGame* UWxSaveLibrary::StartNewSaveFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex, TSubclassOf<UWxSaveGame> SpecificClass)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->StartNewSaveFile(SlotName, UserIndex, SpecificClass);
	}

	return nullptr;
}

UWxSaveGame* UWxSaveLibrary::LoadFromFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->LoadFromFile(SlotName, UserIndex);
	}

	return nullptr;
}

bool UWxSaveLibrary::SaveToFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->SaveToFile(SlotName, UserIndex);
	}

	return false;
}

void UWxSaveLibrary::TravelFromSaveFile(const UObject* WorldContextObject)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->TravelFromSaveFile();
	}
}

bool UWxSaveLibrary::DoesSaveFileExist(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	if (const UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->DoesSaveFileExist(SlotName, UserIndex);
	}

	return false;
}

bool UWxSaveLibrary::DeleteSaveFile(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	if (UWxSaveGameSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->DeleteSaveFile(SlotName, UserIndex);
	}

	return false;
}
