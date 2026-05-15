// Copyright Woogle. All Rights Reserved.

#include "WxSaveLibrary.h"

#include "GameFramework/PlayerController.h"
#include "WxSaveSubsystem.h"

void UWxSaveLibrary::SaveGame(APlayerController* PlayerController)
{
	if (UWxSaveSubsystem* Subsystem = UWxSaveSubsystem::Get(PlayerController))
	{
		Subsystem->SaveGame(PlayerController);
	}
}

bool UWxSaveLibrary::RestartFromSave(APlayerController* PlayerController)
{
	if (UWxSaveSubsystem* Subsystem = UWxSaveSubsystem::Get(PlayerController))
	{
		return Subsystem->RestartFromSave(PlayerController);
	}
	return false;
}

bool UWxSaveLibrary::HasSavedGame(const UObject* WorldContextObject)
{
	if (const UWxSaveSubsystem* Subsystem = UWxSaveSubsystem::Get(WorldContextObject))
	{
		return Subsystem->HasSavedGame();
	}
	return false;
}

bool UWxSaveLibrary::DeleteSavedGame(const UObject* WorldContextObject)
{
	if (UWxSaveSubsystem* Subsystem = UWxSaveSubsystem::Get(WorldContextObject))
	{
		return Subsystem->DeleteSavedGame();
	}
	return false;
}

void UWxSaveLibrary::BeginNewGame(const UObject* WorldContextObject)
{
	if (UWxSaveSubsystem* Subsystem = UWxSaveSubsystem::Get(WorldContextObject))
	{
		Subsystem->BeginNewGame();
	}
}
