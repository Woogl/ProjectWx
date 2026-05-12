// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxSaveGame.generated.h"

/**
 * 본 게임의 SlotData. UWxGameSaveLibrary 가 슬롯 IO 시 직접 인스턴스화/직렬화한다.
 */
UCLASS()
class UWxSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FTransform LastCheckpointTransform = FTransform::Identity;

	UPROPERTY()
	bool bHasCheckpoint = false;
};
