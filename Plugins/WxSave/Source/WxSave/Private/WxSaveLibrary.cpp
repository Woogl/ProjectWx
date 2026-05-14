// Copyright Woogle. All Rights Reserved.

#include "WxSaveLibrary.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WxPlayerSave.h"
#include "WxSaveDeveloperSettings.h"

void UWxSaveLibrary::SavePlayerLocation(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	const UWorld* World = PlayerController->GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	const APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return;
	}

	const FString SlotName = GetSlotName();
	UWxPlayerSave* Slot = LoadOrCreateSlot(SlotName);
	if (!Slot)
	{
		return;
	}

	Slot->PlayerTransform = Pawn->GetActorTransform();
	Slot->bHasSavedLocation = true;

	UGameplayStatics::AsyncSaveGameToSlot(Slot, SlotName, 0);
}

void UWxSaveLibrary::RestartPlayer(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	UWorld* World = PlayerController->GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode)
	{
		return;
	}

	// 기존 Pawn 폐기. APawn::Destroyed 가 PC 의 UnPossess 를 자동 호출한다.
	if (APawn* OldPawn = PlayerController->GetPawn())
	{
		OldPawn->Destroy();
	}

	const UWxPlayerSave* Slot = LoadOrCreateSlot(GetSlotName());
	if (Slot && Slot->bHasSavedLocation)
	{
		GameMode->RestartPlayerAtTransform(PlayerController, Slot->PlayerTransform);
	}
	else
	{
		GameMode->RestartPlayer(PlayerController);
	}
}

bool UWxSaveLibrary::HasSavedPlayerLocation()
{
	const UWxPlayerSave* Slot = LoadOrCreateSlot(GetSlotName());
	return Slot && Slot->bHasSavedLocation;
}

bool UWxSaveLibrary::DeletePlayerSave()
{
	const FString SlotName = GetSlotName();
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return false;
	}

	return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

FString UWxSaveLibrary::GetSlotName()
{
	return GetDefault<UWxSaveDeveloperSettings>()->PlayerSlotName;
}

UWxPlayerSave* UWxSaveLibrary::LoadOrCreateSlot(const FString& SlotName)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		if (UWxPlayerSave* Existing = Cast<UWxPlayerSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			return Existing;
		}
	}
	return Cast<UWxPlayerSave>(UGameplayStatics::CreateSaveGameObject(UWxPlayerSave::StaticClass()));
}
