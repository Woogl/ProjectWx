// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Items/WxItemDefinition.h"

#include "WxInventoryDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wx Inventory Settings"))
class WXINVENTORY_API UWxInventoryDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxInventoryDeveloperSettings();

	/** 아이템 등급별 UI 표시 색상. */
	UPROPERTY(Config, EditAnywhere, Category = "Display")
	TMap<EWxItemGrade, FLinearColor> ItemGradeColors;

	/** 매핑이 없는 등급은 White 를 반환한다. */
	FLinearColor GetItemGradeColor(EWxItemGrade Grade) const;
};
