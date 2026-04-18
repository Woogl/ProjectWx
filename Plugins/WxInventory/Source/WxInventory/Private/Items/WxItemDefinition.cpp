// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemDefinition.h"

UWxItemDefinition::UWxItemDefinition()
	: Grade(EWxItemGrade::Common)
	, MaxCounts(1)
{
}

FPrimaryAssetId UWxItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("WxItem"), GetFName());
}
