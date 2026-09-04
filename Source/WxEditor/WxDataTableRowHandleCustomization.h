// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyEditorModule.h"

class IPropertyHandle;
class IPropertyUtilities;
class SToolTip;
class UDataTable;
class UScriptStruct;
struct FAssetData;

/** WxPreviewRow 메타데이터가 지정된 FDataTableRowHandle 만 전체 행 미리보기를 사용한다. */
class FWxDataTableRowHandleTypeIdentifier : public IPropertyTypeIdentifier
{
public:
	virtual bool IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const override;
};

/** 테이블·행 선택기를 헤더에 두고, 펼친 자식 영역에 선택한 행의 모든 데이터 테이블 칼럼을 읽기 전용으로 표시한다. */
class FWxDataTableRowHandleCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	enum class EWxValueAccess : uint8
	{
		Success,
		MultipleValues,
		Fail,
	};

	FString HandleGetDataTablePath() const;
	void HandleDataTableSelected(const FAssetData& AssetData);
	bool HandleShouldFilterAsset(const FAssetData& AssetData);
	void HandleGetRowStrings(TArray<TSharedPtr<FString>>& OutStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const;
	FString HandleGetRowValueString() const;
	void HandleRowSelected(const FString& RowValue);
	void HandlePropertyChanged();
	void HandleSearchForReferences();
	bool HandleIsEditable() const;
	bool HandleDiffersFromDefault() const;
	void HandleResetToDefault();

	EWxValueAccess GetCurrentValue(const UDataTable*& OutDataTable, FName& OutRowName) const;
	bool GetCommonDataTable(const UDataTable*& OutDataTable) const;
	void RequestRefresh() const;

	TSharedPtr<IPropertyHandle> PropertyHandle;
	TWeakPtr<IPropertyUtilities> PropertyUtilities;
	FName RowTypeFilter;
	UScriptStruct* RowFilterStruct = nullptr;
};
