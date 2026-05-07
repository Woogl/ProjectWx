// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemDefinition.h"

#include "Items/WxItemFragment.h"

UWxItemDefinition::UWxItemDefinition()
	: Grade(EWxItemGrade::Common)
	, Category(EWxItemCategory::None)
{
}

FPrimaryAssetId UWxItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("WxItem"), GetFName());
}

EWxItemCategory UWxItemDefinition::GetItemCategory() const
{
	return Category;
}

const UWxItemFragment* UWxItemDefinition::FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const
{
	if (!FragmentClass)
	{
		return nullptr;
	}

	for (UWxItemFragment* Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	return nullptr;
}
