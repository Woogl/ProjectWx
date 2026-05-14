// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxSaveDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wx Save Settings"))
class WXSAVE_API UWxSaveDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxSaveDeveloperSettings();

	/** 플레이어 위치 저장에 사용할 슬롯명. */
	UPROPERTY(Config, EditAnywhere, Category = "Slot")
	FString PlayerSlotName = TEXT("WxPlayerSlot");
};
