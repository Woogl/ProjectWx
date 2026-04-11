// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxWorldDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wx World Settings"))
class WXWORLD_API UWxWorldDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxWorldDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Spawner")
	TMap<TSoftClassPtr<AActor>, TSoftObjectPtr<UTexture2D>> SpawnerClassIcons;

	UTexture2D* FindSpawnerIconForClass(UClass* ActorClass) const;
};
