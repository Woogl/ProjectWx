// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxUIDeveloperSettings.generated.h"

class UWxPrimaryGameLayout;
class UWxGamePopup;

UCLASS(Config = Game, DefaultConfig)
class WXUI_API UWxUIDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxUIDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	TSoftClassPtr<UWxPrimaryGameLayout> LayoutClass;

	UPROPERTY(Config, EditAnywhere, Category = "Popup")
	TSoftClassPtr<UWxGamePopup> ConfirmationPopupClass;
};
