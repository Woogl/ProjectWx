// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxUIDeveloperSettings.generated.h"

class UWxPrimaryGameLayout;
class UWxGamePopup;
class UWxActivatableWidget;

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

	/** 플레이어 캐릭터에 빙의하면 Game 레이어에 띄울 HUD. 미지정이면 동작 없음. */
	UPROPERTY(Config, EditAnywhere, Category = "Screen")
	TSoftClassPtr<UWxActivatableWidget> GameHUDClass;

	/** 빙의된 캐릭터 사망 시 Menu 레이어에 띄울 위젯. 미지정이면 동작 없음. */
	UPROPERTY(Config, EditAnywhere, Category = "Screen")
	TSoftClassPtr<UWxActivatableWidget> DeathScreenClass;

	/** 대화 세션이 열리면 Game 레이어에 띄울 대화 위젯. 미지정이면 동작 없음. */
	UPROPERTY(Config, EditAnywhere, Category = "Screen")
	TSoftClassPtr<UWxActivatableWidget> DialogueScreenClass;
};
