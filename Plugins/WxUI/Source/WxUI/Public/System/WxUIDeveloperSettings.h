// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxUIDeveloperSettings.generated.h"

class UWxPrimaryGameLayout;
class UWxGameDialog;

UCLASS(Config = Game, DefaultConfig)
class WXUI_API UWxUIDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxUIDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Layout")
	TSoftClassPtr<UWxPrimaryGameLayout> LayoutClass;

	/** 확인 팝업에 사용할 다이얼로그 위젯 클래스. */
	UPROPERTY(Config, EditAnywhere, Category = "Dialog")
	TSoftClassPtr<UWxGameDialog> ConfirmationDialogClass;

	/** 에러 팝업에 사용할 다이얼로그 위젯 클래스. */
	UPROPERTY(Config, EditAnywhere, Category = "Dialog")
	TSoftClassPtr<UWxGameDialog> ErrorDialogClass;
};
