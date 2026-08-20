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

	// 서브카테고리(예: "Wx|Input") 에 EditCategory 를 호출하면 중첩 렌더링과 별도로 평면 카테고리가 한 번 더 그려져 중복된다.
	// 그래서 최상위 세그먼트만 정렬 대상으로 삼는다.
	TSet<FName> TopLevelCategories;
	for (const FName& CategoryName : CategoryNames)
	{
		const FString CategoryStr = CategoryName.ToString();
		if (!CategoryStr.StartsWith(WxCategory::CategoryPrefix))
		{
			continue;
		}

		int32 PipeIndex = INDEX_NONE;
		if (CategoryStr.FindChar(TEXT('|'), PipeIndex))
		{
			TopLevelCategories.Add(FName(*CategoryStr.Left(PipeIndex)));
		}
		else
		{
			TopLevelCategories.Add(CategoryName);
		}
	}

	TArray<FName> SortedNames = TopLevelCategories.Array();
	SortedNames.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});

	int32 SortOrder = WxCategory::BaseSortOrder;
	for (const FName& CategoryName : SortedNames)
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(CategoryName);
		Category.SetSortOrder(SortOrder++);
	}
}
