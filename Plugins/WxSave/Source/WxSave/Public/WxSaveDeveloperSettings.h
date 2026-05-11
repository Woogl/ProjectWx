// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxSaveDeveloperSettings.generated.h"

class UWxSlotData;
class UWxSlotInfo;

/**
 * WxSave 의 Project Settings 항목. 게임 측 SlotData/SlotInfo 서브클래스를 여기서 지정한다.
 * Project Settings → Plugins → WxSave 에 노출된다.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="WxSave"))
class WXSAVE_API UWxSaveDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxSaveDeveloperSettings();
	
	UPROPERTY(EditAnywhere, Config, Category="Save")
	TSubclassOf<UWxSlotData> SlotDataClass;

	UPROPERTY(EditAnywhere, Config, Category="Save")
	TSubclassOf<UWxSlotInfo> SlotInfoClass;
};
