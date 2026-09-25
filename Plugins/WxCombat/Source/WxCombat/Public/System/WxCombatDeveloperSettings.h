// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxCombatDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wx Combat Settings"))
class WXCOMBAT_API UWxCombatDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxCombatDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Damage")
	float DefenseConstant;
};
