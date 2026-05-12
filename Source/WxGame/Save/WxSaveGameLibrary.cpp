// Copyright Woogle. All Rights Reserved.

#include "Save/WxSaveGameLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/WxSaveGame.h"
#include "System/WxSpawnerSubsystem.h"

static const FString DefaultSlotName = TEXT("WxSaveSlot");

static UWxSaveGame* LoadOrCreateSlotData()
{
	if (UGameplayStatics::DoesSaveGameExist(DefaultSlotName, 0))
	{
		if (UWxSaveGame* Existing = Cast<UWxSaveGame>(UGameplayStatics::LoadGameFromSlot(DefaultSlotName, 0)))
		{
			return Existing;
		}
	}
	return Cast<UWxSaveGame>(UGameplayStatics::CreateSaveGameObject(UWxSaveGame::StaticClass()));
}

void UWxSaveGameLibrary::SaveLastCheckpoint(const UObject* WorldContextObject, const FTransform& Transform)
{
	if (!GEngine || !WorldContextObject)
	{
		return;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	UWxSaveGame* SlotData = LoadOrCreateSlotData();
	if (!SlotData)
	{
		return;
	}

	SlotData->LastCheckpointTransform = Transform;
	SlotData->bHasCheckpoint = true;

	UGameplayStatics::AsyncSaveGameToSlot(SlotData, DefaultSlotName, 0);
}

FTransform UWxSaveGameLibrary::GetLastCheckpoint(const UObject* WorldContextObject)
{
	const UWxSaveGame* SlotData = LoadOrCreateSlotData();
	return (SlotData && SlotData->bHasCheckpoint) ? SlotData->LastCheckpointTransform : FTransform::Identity;
}

bool UWxSaveGameLibrary::HasValidCheckpoint(const UObject* WorldContextObject)
{
	const UWxSaveGame* SlotData = LoadOrCreateSlotData();
	return SlotData && SlotData->bHasCheckpoint;
}

void UWxSaveGameLibrary::RestartPlayerAtLastCheckpoint(const UObject* WorldContextObject, int32 PlayerIndex)
{
	if (!GEngine || !WorldContextObject)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
	if (!PlayerController)
	{
		return;
	}

	// 기존 Pawn 폐기. APawn::Destroyed 가 PC 의 UnPossess 를 자동 호출한다.
	if (APawn* OldPawn = PlayerController->GetPawn())
	{
		OldPawn->Destroy();
	}

	const UWxSaveGame* SlotData = LoadOrCreateSlotData();
	if (SlotData && SlotData->bHasCheckpoint)
	{
		GameMode->RestartPlayerAtTransform(PlayerController, SlotData->LastCheckpointTransform);
	}
	else
	{
		// 체크포인트 미기록 시 기본 PlayerStart 에서 스폰 (GameMode 의 ChoosePlayerStart 경로)
		GameMode->RestartPlayer(PlayerController);
	}

	// 처치된 모든 Spawner 부활
	if (UWxSpawnerSubsystem* SpawnerSubsystem = World->GetSubsystem<UWxSpawnerSubsystem>())
	{
		SpawnerSubsystem->RespawnAll();
	}
}
