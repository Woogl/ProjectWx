// Copyright Woogle. All Rights Reserved.

#include "WxObjectDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"

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
	CustomizeWxCategory(DetailBuilder);
}

void FWxObjectDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	if (Inner.IsValid())
	{
		Inner->CustomizeDetails(DetailBuilder);
	}
	CustomizeWxCategory(*DetailBuilder);
}

void FWxObjectDetails::PendingDelete()
{
	if (Inner.IsValid())
	{
		Inner->PendingDelete();
	}
}

void FWxObjectDetails::CustomizeWxCategory(IDetailLayoutBuilder& DetailBuilder)
{
	// 없는 카테고리를 EditCategory 로 만들어 두지 않는다.
	TArray<FName> CategoryNames;
	DetailBuilder.GetCategoryNames(CategoryNames);
	if (!CategoryNames.Contains(WxCategoryName))
	{
		return;
	}

	// 다른 커스터마이제이션이 먼저 편집한 카테고리는 EditCategory 가 정렬값을 갱신하지 않으므로 직접 지정한다.
	IDetailCategoryBuilder& WxCategory = DetailBuilder.EditCategory(WxCategoryName);
	WxCategory.SetSortOrder(WxObjectDetails::WxCategorySortOrder);

	// 하위 카테고리 행은 프로퍼티 없는 핸들로 온다. 이름이 겹치는 두 번째부터 숨기면 남은 첫 행이 병합된 내용을 그대로 보여준다.
	TArray<TSharedRef<IPropertyHandle>> DefaultProperties;
	WxCategory.GetDefaultProperties(DefaultProperties);
	TArray<FString> SeenSubcategoryNames;
	for (const TSharedRef<IPropertyHandle>& Handle : DefaultProperties)
	{
		if (Handle->GetProperty() != nullptr)
		{
			continue;
		}

		const FString SubcategoryName = Handle->GetPropertyDisplayName().ToString();
		if (SeenSubcategoryNames.Contains(SubcategoryName))
		{
			DetailBuilder.HideProperty(Handle);
		}
		else
		{
			SeenSubcategoryNames.Add(SubcategoryName);
		}
	}
}
