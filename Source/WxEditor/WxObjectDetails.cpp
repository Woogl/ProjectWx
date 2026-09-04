// Copyright Woogle. All Rights Reserved.

#include "WxObjectDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

const FName FWxObjectDetails::WxCategoryName(TEXT("Wx"));

namespace WxObjectDetails
{
	/** 엔진이 커스터마이제이션 뒤에 만드는 Favorites(0) 바로 다음, 그 외 카테고리(ECategoryPriority*1000 이상, Transform 은 1000) 앞. */
	static const int32 WxCategorySortOrder = 1;
}

TSharedRef<IDetailCustomization> FWxObjectDetails::MakeInstance(FOnGetDetailCustomizationInstance InnerFactory)
{
	TSharedRef<FWxObjectDetails> Instance = MakeShared<FWxObjectDetails>();
	if (InnerFactory.IsBound())
	{
		Instance->Inner = InnerFactory.Execute();
	}
	return Instance;
}

void FWxObjectDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (Inner.IsValid())
	{
		Inner->CustomizeDetails(DetailBuilder);
	}
	MoveWxCategoryToTop(DetailBuilder);
}

void FWxObjectDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	if (Inner.IsValid())
	{
		Inner->CustomizeDetails(DetailBuilder);
	}
	MoveWxCategoryToTop(*DetailBuilder);
}

void FWxObjectDetails::PendingDelete()
{
	if (Inner.IsValid())
	{
		Inner->PendingDelete();
	}
}

void FWxObjectDetails::MoveWxCategoryToTop(IDetailLayoutBuilder& DetailBuilder)
{
	// 없는 카테고리를 EditCategory 로 만들어 두지 않는다.
	TArray<FName> CategoryNames;
	DetailBuilder.GetCategoryNames(CategoryNames);
	if (!CategoryNames.Contains(WxCategoryName))
	{
		return;
	}

	// 다른 커스터마이제이션이 먼저 편집한 카테고리는 EditCategory 가 정렬값을 갱신하지 않으므로 직접 지정한다.
	DetailBuilder.EditCategory(WxCategoryName).SetSortOrder(WxObjectDetails::WxCategorySortOrder);
}
