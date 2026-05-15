// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "WxSaveGameSubsystem.h"

AActor* AWxGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	UWxSaveGameSubsystem* SaveSubsystem = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SaveSubsystem = GameInstance->GetSubsystem<UWxSaveGameSubsystem>();
	}

	FTransform RespawnTransform;
	if (SaveSubsystem && SaveSubsystem->TryGetPlayerRespawnTransform(RespawnTransform))
	{
		if (RespawnPlayerStart)
		{
			RespawnPlayerStart->Destroy();
			RespawnPlayerStart = nullptr;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		RespawnPlayerStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), RespawnTransform, SpawnParams);
		if (RespawnPlayerStart)
		{
			return RespawnPlayerStart;
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}
