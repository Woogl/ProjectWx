// Copyright Woogle. All Rights Reserved.

#include "WxDataTableRowHandleCustomization.h"

#include "AssetRegistry/AssetData.h"
#include "DataTableEditorUtils.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "UObject/Class.h"
#include "UObject/NameTypes.h"
#include "Widgets/Layout/SWrapBox.h"

#define LOCTEXT_NAMESPACE "WxDataTableRowHandleCustomization"

namespace WxDataTableRowHandleCustomization
{
	static const FName RowTypeName(TEXT("RowType"));
	static const FName RowStructureTagName(TEXT("RowStructure"));
}

TSharedRef<IPropertyTypeCustomization> FWxDataTableRowHandleCustomization::MakeInstance()
{
	return MakeShared<FWxDataTableRowHandleCustomization>();
}

void FWxDataTableRowHandleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	using namespace WxDataTableRowHandleCustomization;

	RowTypeFilter = NAME_None;
	RowFilterStruct = nullptr;
	if (InPropertyHandle->HasMetaData(RowTypeName))
	{
		const FString& RowType = InPropertyHandle->GetMetaData(RowTypeName);
		RowTypeFilter = FName(*RowType);
		RowFilterStruct = UClass::TryFindTypeSlow<UScriptStruct>(RowType);
	}

	DataTablePropertyHandle = InPropertyHandle->GetChildHandle(TEXT("DataTable"));
	RowNamePropertyHandle = InPropertyHandle->GetChildHandle(TEXT("RowName"));
	if (!DataTablePropertyHandle.IsValid() || !DataTablePropertyHandle->IsValidHandle()
		|| !RowNamePropertyHandle.IsValid() || !RowNamePropertyHandle->IsValidHandle())
	{
		return;
	}

	// 구조체째 바뀌는 경로(붙여넣기·되돌리기)도 남은 행 이름을 정리해야 하므로 양쪽에 건다.
	const FSimpleDelegate DataTableChangedDelegate = FSimpleDelegate::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleDataTableChanged);
	InPropertyHandle->SetOnPropertyValueChanged(DataTableChangedDelegate);
	DataTablePropertyHandle->SetOnPropertyValueChanged(DataTableChangedDelegate);

	FPropertyComboBoxArgs ComboArgs(
		RowNamePropertyHandle,
		FOnGetPropertyComboBoxStrings::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleGetRowStrings),
		FOnGetPropertyComboBoxValue::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleGetRowValueString));
	ComboArgs.ShowSearchForItemCount = 1;

	HeaderRow
	.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(800.f)
	[
		// 파라미터 패널처럼 값 칸이 좁으면 두 위젯이 줄바꿈된다 — 잘린 채 남지 않게 하는 엔진과 같은 배치다.
		SNew(SWrapBox)
		.UseAllottedSize(true)
		.Orientation(Orient_Horizontal)
		.HAlign(HAlign_Fill)
		+ SWrapBox::Slot()
		[
			SNew(SObjectPropertyEntryBox)
			.PropertyHandle(DataTablePropertyHandle)
			.AllowedClass(UDataTable::StaticClass())
			.OnShouldFilterAsset(this, &FWxDataTableRowHandleCustomization::HandleShouldFilterAsset)
		]
		+ SWrapBox::Slot()
		[
			PropertyCustomizationHelpers::MakePropertyComboBox(ComboArgs)
		]
	];

	FDataTableEditorUtils::AddSearchForReferencesContextMenu(
		HeaderRow,
		FExecuteAction::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleSearchForReferences));
}

void FWxDataTableRowHandleCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// 자식을 비워야 확장 화살표가 사라져 접힘 자체가 없어진다.
}

void FWxDataTableRowHandleCustomization::HandleDataTableChanged()
{
	UDataTable* DataTable = nullptr;
	FName RowName;
	if (!GetCurrentValue(DataTable, RowName))
	{
		return;
	}

	if (!DataTable || !DataTable->FindRowUnchecked(RowName))
	{
		RowNamePropertyHandle->SetValue(FName());
	}
}

bool FWxDataTableRowHandleCustomization::HandleShouldFilterAsset(const FAssetData& AssetData)
{
	using namespace WxDataTableRowHandleCustomization;

	if (RowTypeFilter.IsNone())
	{
		return false;
	}

	FString RowStructure;
	if (AssetData.GetTagValue<FString>(RowStructureTagName, RowStructure))
	{
		if (RowStructure == RowTypeFilter.ToString())
		{
			return false;
		}

		// 태그는 구조체 짧은 이름뿐이라 파생 행 구조체는 느린 검색으로만 가려낼 수 있다.
		const UScriptStruct* RowStruct = UClass::TryFindTypeSlow<UScriptStruct>(RowStructure);
		if (RowStruct && RowFilterStruct && RowStruct->IsChildOf(RowFilterStruct))
		{
			return false;
		}
	}

	return true;
}

void FWxDataTableRowHandleCustomization::HandleGetRowStrings(TArray<TSharedPtr<FString>>& OutStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const
{
	// 테이블이 같고 행 이름만 다중 값일 때도 목록은 보여야 하므로 반환값을 보지 않는다.
	UDataTable* DataTable = nullptr;
	FName UnusedRowName;
	GetCurrentValue(DataTable, UnusedRowName);
	if (!DataTable)
	{
		return;
	}

	TArray<FName> RowNames;
	DataTable->GetRowMap().GetKeys(RowNames);
	RowNames.Sort(FNameLexicalLess());

	for (const FName& RowName : RowNames)
	{
		OutStrings.Add(MakeShared<FString>(RowName.ToString()));
		OutRestrictedItems.Add(false);
	}
}

FString FWxDataTableRowHandleCustomization::HandleGetRowValueString() const
{
	if (!RowNamePropertyHandle.IsValid() || !RowNamePropertyHandle->IsValidHandle())
	{
		return FString();
	}

	FName RowName;
	if (RowNamePropertyHandle->GetValue(RowName) == FPropertyAccess::MultipleValues)
	{
		return LOCTEXT("MultipleValues", "Multiple Values").ToString();
	}

	return RowName.IsNone() ? LOCTEXT("NoRow", "None").ToString() : RowName.ToString();
}

void FWxDataTableRowHandleCustomization::HandleSearchForReferences()
{
	UDataTable* DataTable = nullptr;
	FName RowName;
	if (!GetCurrentValue(DataTable, RowName) || !DataTable)
	{
		return;
	}

	TArray<FAssetIdentifier> AssetIdentifiers;
	AssetIdentifiers.Add(FAssetIdentifier(DataTable, RowName));

	FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
}

bool FWxDataTableRowHandleCustomization::GetCurrentValue(UDataTable*& OutDataTable, FName& OutRowName) const
{
	if (!DataTablePropertyHandle.IsValid() || !DataTablePropertyHandle->IsValidHandle()
		|| !RowNamePropertyHandle.IsValid() || !RowNamePropertyHandle->IsValidHandle())
	{
		return false;
	}

	UObject* SourceDataTable = nullptr;
	if (DataTablePropertyHandle->GetValue(SourceDataTable) != FPropertyAccess::Success)
	{
		return false;
	}

	OutDataTable = Cast<UDataTable>(SourceDataTable);
	return RowNamePropertyHandle->GetValue(OutRowName) == FPropertyAccess::Success;
}

#undef LOCTEXT_NAMESPACE
