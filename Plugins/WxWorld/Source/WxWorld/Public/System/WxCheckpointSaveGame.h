// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxCheckpointSaveGame.generated.h"

/** 싱글플레이 체크포인트의 레벨과 위치·회전을 디스크에 저장한다. */
UCLASS()
class WXWORLD_API UWxCheckpointSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static bool SaveCheckpoint(const UWorld* World, const FTransform& Transform);
	static bool TryGetCheckpoint(const UWorld* World, FTransform& OutTransform);
	static bool ResetCheckpoint(const UWorld* World);

private:
	static FString GetSlotName();

	UPROPERTY(SaveGame)
	FName LevelPackage;

	UPROPERTY(SaveGame)
	FTransform RespawnTransform = FTransform::Identity;
};
