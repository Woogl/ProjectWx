// Copyright Woogle. All Rights Reserved.

#include "Inventory/WxItemDefinition.h"

#include "Inventory/WxItemFragment.h"

FGameplayTag UWxItemDefinition::GetItemCategory() const
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
