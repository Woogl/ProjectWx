// Copyright Woogle. All Rights Reserved.

#include "Save/WxGameSaveLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/WxGameSaveSubsystem.h"

namespace
{
	UWxGameSaveSubsystem* FindSaveSubsystem(const UObject* WorldContextObject)
	{
		if (!GEngine || !WorldContextObject)
		{
			return nullptr;
		}

		const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		if (!World)
		{
			return nullptr;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		return GameInstance ? GameInstance->GetSubsystem<UWxGameSaveSubsystem>() : nullptr;
	}
}

void UWxGameSaveLibrary::SetLastCheckpoint(const UObject* WorldContextObject, const FTransform& Transform)
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

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UWxGameSaveSubsystem* Save = GameInstance->GetSubsystem<UWxGameSaveSubsystem>())
		{
			Save->SetLastCheckpoint(Transform);
		}
	}
}

bool UWxGameSaveLibrary::HasValidCheckpoint(const UObject* WorldContextObject)
{
	const UWxGameSaveSubsystem* Save = FindSaveSubsystem(WorldContextObject);
	return Save && Save->HasValidCheckpoint();
}

FTransform UWxGameSaveLibrary::GetLastCheckpoint(const UObject* WorldContextObject)
{
	const UWxGameSaveSubsystem* Save = FindSaveSubsystem(WorldContextObject);
	return Save ? Save->GetLastCheckpoint() : FTransform::Identity;
}

void UWxGameSaveLibrary::RestartLocalPlayerAtLastCheckpoint(const UObject* WorldContextObject)
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

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!PlayerController)
	{
		return;
	}

	// 기존 Pawn 폐기. APawn::Destroyed 가 PC 의 UnPossess 를 자동 호출한다.
	if (APawn* OldPawn = PlayerController->GetPawn())
	{
		OldPawn->Destroy();
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	const UWxGameSaveSubsystem* Save = GameInstance ? GameInstance->GetSubsystem<UWxGameSaveSubsystem>() : nullptr;

	if (Save && Save->HasValidCheckpoint())
	{
		GameMode->RestartPlayerAtTransform(PlayerController, Save->GetLastCheckpoint());
	}
	else
	{
		// 체크포인트 미기록 시 기본 PlayerStart 에서 스폰 (GameMode 의 ChoosePlayerStart 경로)
		GameMode->RestartPlayer(PlayerController);
	}
}
