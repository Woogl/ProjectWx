// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemDefinition.h"

#include "Items/WxItemFragment.h"

UWxItemDefinition::UWxItemDefinition()
	: Category(EWxItemCategory::None)
{
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
