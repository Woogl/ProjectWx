// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxSaveSettings.generated.h"

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "Wx Save", CategoryName = "Wx"))
class WXSAVE_API UWxSaveSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = "Persistence")
	bool bAutoSaveWhenLeavingMap = true;

	UPROPERTY(config, EditAnywhere, Category = "Persistence")
	bool bRestorePawnTransform = true;

	UPROPERTY(config, EditAnywhere, Category = "Persistence")
	bool bRestoreControlRotation = true;

	/** 저장을 허용할 FMassFragment 파생 UScriptStruct의 전체 경로. */
	UPROPERTY(config, EditAnywhere, Category = "Mass", meta = (GetOptions = "GetMassFragmentOptions"))
	TSet<FName> MassFragmentsToSerialize;

protected:
	UFUNCTION()
	TArray<FString> GetMassFragmentOptions() const;
};
