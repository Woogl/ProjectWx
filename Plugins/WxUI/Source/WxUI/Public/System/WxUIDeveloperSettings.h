// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxUIDeveloperSettings.generated.h"

class UWxPrimaryGameLayout;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wx UI Settings"))
class WXUI_API UWxUIDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxUIDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	TSoftClassPtr<UWxPrimaryGameLayout> LayoutClass;
};
