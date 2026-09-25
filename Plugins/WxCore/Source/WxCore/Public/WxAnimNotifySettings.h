// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxAnimNotifySettings.generated.h"

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Wx Anim Notify Settings"))
class WXCORE_API UWxAnimNotifySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxAnimNotifySettings();

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor AttackColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor AbilityFlowColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor EffectColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor MovementColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor PresentationColor;

	UPROPERTY(Config, EditAnywhere, Category = "Colors", meta = (HideAlphaChannel))
	FLinearColor MiscColor;
#endif
};
