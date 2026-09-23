// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxFrontEndDeveloperSettings.generated.h"

class APawn;

UCLASS(Config = Game, DefaultConfig)
class WXGAME_API UWxFrontEndDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxFrontEndDeveloperSettings();

	bool HasSelectableOptions() const;

	UPROPERTY(Config, EditAnywhere, Category = "Options")
	TArray<TSoftClassPtr<APawn>> CharacterOptions;

	UPROPERTY(Config, EditAnywhere, Category = "Options")
	TArray<TSoftObjectPtr<UWorld>> LevelOptions;
};
