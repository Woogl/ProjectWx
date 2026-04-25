// Copyright Woogle. All Rights Reserved.

#include "WxCategoryDetailCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

namespace WxCategory
{
	static const TCHAR* CategoryPrefix = TEXT("Wx");

	// Transform 카테고리(기본 SortOrder 가 작은 값)보다도 위에 두기 위해 충분히 작은 베이스를 사용한다.
	static constexpr int32 BaseSortOrder = -10000;
}

TSharedRef<IDetailCustomization> FWxCategoryDetailCustomization::MakeInstance()
{
	return MakeShared<FWxCategoryDetailCustomization>();
}

void FWxCategoryDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<FName> CategoryNames;
	DetailBuilder.GetCategoryNames(CategoryNames);

	CategoryNames.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});

	int32 SortOrder = WxCategory::BaseSortOrder;
	for (const FName& CategoryName : CategoryNames)
	{
		if (!CategoryName.ToString().StartsWith(WxCategory::CategoryPrefix))
		{
			continue;
		}

		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(CategoryName);
		Category.SetSortOrder(SortOrder++);
	}
}
