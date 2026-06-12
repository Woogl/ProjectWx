// Copyright Woogle. All Rights Reserved.

#include "System/WxInventoryDeveloperSettings.h"

UWxInventoryDeveloperSettings::UWxInventoryDeveloperSettings()
{
	CategoryName = TEXT("Wx");

	ItemGradeColors.Add(EWxItemGrade::Common, FLinearColor(0.7f, 0.7f, 0.7f));
	ItemGradeColors.Add(EWxItemGrade::Rare, FLinearColor(0.2f, 0.45f, 1.0f));
	ItemGradeColors.Add(EWxItemGrade::Epic, FLinearColor(0.6f, 0.25f, 0.9f));
	ItemGradeColors.Add(EWxItemGrade::Legendary, FLinearColor(1.0f, 0.55f, 0.1f));
}

FLinearColor UWxInventoryDeveloperSettings::GetItemGradeColor(EWxItemGrade Grade) const
{
	if (const FLinearColor* FoundColor = ItemGradeColors.Find(Grade))
	{
		return *FoundColor;
	}
	return FLinearColor::White;
}
