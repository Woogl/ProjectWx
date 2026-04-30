// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemDefinition.h"

#include "Items/WxItemFragment.h"

UWxItemDefinition::UWxItemDefinition()
	: Grade(EWxItemGrade::Common)
	, MaxCounts(1)
{
}

FPrimaryAssetId UWxItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("WxItem"), GetFName());
}

EWxItemCategory UWxItemDefinition::GetItemCategory() const
{
	if (FindFragment<FWxItemFragment_Equipment>())
	{
		return EWxItemCategory::Equipment;
	}

	if (FindFragment<FWxItemFragment_Consumable>())
	{
		return EWxItemCategory::Consumable;
	}

	if (FindFragment<FWxItemFragment_Currency>())
	{
		return EWxItemCategory::Currency;
	}

	return EWxItemCategory::None;
}
