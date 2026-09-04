// Copyright Woogle. All Rights Reserved.

#include "WxDataTableRowHandleCustomization.h"

#include "AssetRegistry/AssetData.h"
#include "DataTableEditorUtils.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Framework/Commands/UIAction.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WxDataTableRowHandleCustomization"

namespace WxDataTableRowHandleCustomization
{
	static const FName PreviewMetadataName(TEXT("WxPreviewRow"));
	static const FName RowStructureTagName(TEXT("RowStructure"));
	static const FName RowTypeMetadataName(TEXT("RowType"));

	const FProperty* FindMetaCarrier(const FProperty* Property, FName MetaName)
	{
		if (!Property)
		{
			return nullptr;
		}
		if (Property->HasMetaData(MetaName))
		{
			return Property;
		}

		const FArrayProperty* OwnerArray = Property->GetOwner<FArrayProperty>();
		if (OwnerArray && OwnerArray->HasMetaData(MetaName))
		{
			return OwnerArray;
		}

		return nullptr;
	}
}

bool FWxDataTableRowHandleTypeIdentifier::IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const
{
	using namespace WxDataTableRowHandleCustomization;

	return FindMetaCarrier(PropertyHandle.GetProperty(), PreviewMetadataName) != nullptr;
}

TSharedRef<IPropertyTypeCustomization> FWxDataTableRowHandleCustomization::MakeInstance()
{
	return MakeShared<FWxDataTableRowHandleCustomization>();
}

void FWxDataTableRowHandleCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	using namespace WxDataTableRowHandleCustomization;

	PropertyHandle = InPropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	RowTypeFilter = NAME_None;
	RowFilterStruct = nullptr;

	if (const FProperty* MetaCarrier = FindMetaCarrier(InPropertyHandle->GetProperty(), RowTypeMetadataName))
	{
		const FString& RowType = MetaCarrier->GetMetaData(RowTypeMetadataName);
		RowTypeFilter = FName(*RowType);
		RowFilterStruct = UClass::TryFindTypeSlow<UScriptStruct>(RowType);
	}

	InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FWxDataTableRowHandleCustomization::HandlePropertyChanged));

	FPropertyComboBoxArgs ComboArgs;
	ComboArgs.OnGetStrings = FOnGetPropertyComboBoxStrings::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleGetRowStrings);
	ComboArgs.OnGetValue = FOnGetPropertyComboBoxValue::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleGetRowValueString);
	ComboArgs.OnValueSelected = FOnPropertyComboBoxValueSelected::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleRowSelected);
	ComboArgs.ShowSearchForItemCount = 1;

	FUIAction CopyAction;
	FUIAction PasteAction;
	InPropertyHandle->CreateDefaultPropertyCopyPasteActions(CopyAction, PasteAction);

	HeaderRow
	.ShouldAutoExpand(false)
	.CopyAction(CopyAction)
	.PasteAction(PasteAction)
	.OverrideResetToDefault(FResetToDefaultOverride::Create(
		TAttribute<bool>::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleDiffersFromDefault),
		FSimpleDelegate::CreateSP(this, &FWxDataTableRowHandleCustomization::HandleResetToDefault)))
	.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(800.f)
	[
		SNew(SWrapBox)
		.UseAllottedSize(true)
		.Orientation(Orient_Horizontal)
		.HAlign(HAlign_Fill)
		.IsEnabled(this, &FWxDataTableRowHandleCustomization::HandleIsEditable)
		+ SWrapBox::Slot()
		[
			SNew(SObjectPropertyEntryBox)
			.ObjectPath(this, &FWxDataTableRowHandleCustomization::HandleGetDataTablePath)
			.AllowedClass(UDataTable::StaticClass())
			.AllowClear(true)
			.DisplayThumbnail(false)
			.OnObjectChanged(this, &FWxDataTableRowHandleCustomization::HandleDataTableSelected)
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
	const UDataTable* DataTable = nullptr;
	FName RowName;
	if (GetCurrentValue(DataTable, RowName) != EWxValueAccess::Success || !DataTable || RowName.IsNone())
	{
		return;
	}

	uint8* SelectedRowData = DataTable->FindRowUnchecked(RowName);
	if (!SelectedRowData || !DataTable->GetRowStruct())
	{
		return;
	}

	TMap<FName, uint8*> SelectedRowMap;
	SelectedRowMap.Add(RowName, SelectedRowData);

	TArray<FDataTableEditorColumnHeaderDataPtr> Columns;
	TArray<FDataTableEditorRowListViewDataPtr> Rows;
	FDataTableEditorUtils::CacheDataForEditing(DataTable->GetRowStruct(), SelectedRowMap, Columns, Rows);
	if (Rows.IsEmpty() || !Rows[0].IsValid())
	{
		return;
	}

	const FDataTableEditorRowListViewDataPtr& SelectedRow = Rows[0];
	const int32 ColumnCount = FMath::Min(Columns.Num(), SelectedRow->CellData.Num());
	for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		const FDataTableEditorColumnHeaderDataPtr& Column = Columns[ColumnIndex];
		if (!Column.IsValid())
		{
			continue;
		}

		const FText CellText = SelectedRow->CellData[ColumnIndex];
		ChildBuilder.AddCustomRow(Column->DisplayName)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(Column->DisplayName)
			.Font(CustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(STextBlock)
			.Text(CellText)
			.ToolTipText(CellText)
			.AutoWrapText(true)
			.Font(CustomizationUtils.GetRegularFont())
		];
	}
}

FString FWxDataTableRowHandleCustomization::HandleGetDataTablePath() const
{
	const UDataTable* DataTable = nullptr;
	return GetCommonDataTable(DataTable) && DataTable ? DataTable->GetPathName() : FString();
}

void FWxDataTableRowHandleCustomization::HandleDataTableSelected(const FAssetData& AssetData)
{
	UDataTable* NewDataTable = Cast<UDataTable>(AssetData.GetAsset());
	const FScopedTransaction Transaction(LOCTEXT("SetDataTable", "Set Data Table Row Handle Table"));
	PropertyHandle->NotifyPreChange();

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	for (void* Raw : RawData)
	{
		if (!Raw)
		{
			continue;
		}

		FDataTableRowHandle* RowHandle = static_cast<FDataTableRowHandle*>(Raw);
		RowHandle->DataTable = NewDataTable;
		if (!NewDataTable || !NewDataTable->FindRowUnchecked(RowHandle->RowName))
		{
			RowHandle->RowName = NAME_None;
		}
	}

	PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	PropertyHandle->NotifyFinishedChangingProperties();
	RequestRefresh();
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
	const UDataTable* DataTable = nullptr;
	if (!GetCommonDataTable(DataTable) || !DataTable)
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
	const UDataTable* DataTable = nullptr;
	FName RowName;
	switch (GetCurrentValue(DataTable, RowName))
	{
	case EWxValueAccess::Success:
		return RowName.IsNone() ? LOCTEXT("NoRow", "None").ToString() : RowName.ToString();
	case EWxValueAccess::MultipleValues:
		return LOCTEXT("MultipleValues", "Multiple Values").ToString();
	default:
		return LOCTEXT("NoRow", "None").ToString();
	}
}

void FWxDataTableRowHandleCustomization::HandleRowSelected(const FString& RowValue)
{
	const FName NewRowName(*RowValue);
	const FScopedTransaction Transaction(LOCTEXT("SetRow", "Set Data Table Row Handle Row"));
	PropertyHandle->NotifyPreChange();

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	for (void* Raw : RawData)
	{
		if (Raw)
		{
			static_cast<FDataTableRowHandle*>(Raw)->RowName = NewRowName;
		}
	}

	PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	PropertyHandle->NotifyFinishedChangingProperties();
	RequestRefresh();
}

void FWxDataTableRowHandleCustomization::HandlePropertyChanged()
{
	RequestRefresh();
}

void FWxDataTableRowHandleCustomization::HandleSearchForReferences()
{
	const UDataTable* DataTable = nullptr;
	FName RowName;
	if (GetCurrentValue(DataTable, RowName) != EWxValueAccess::Success || !DataTable)
	{
		return;
	}

	TArray<FAssetIdentifier> AssetIdentifiers;
	AssetIdentifiers.Add(FAssetIdentifier(const_cast<UDataTable*>(DataTable), RowName));
	FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
}

bool FWxDataTableRowHandleCustomization::HandleIsEditable() const
{
	return PropertyHandle.IsValid() && PropertyHandle->IsValidHandle() && !PropertyHandle->IsEditConst();
}

bool FWxDataTableRowHandleCustomization::HandleDiffersFromDefault() const
{
	return PropertyHandle.IsValid() && PropertyHandle->IsValidHandle() && PropertyHandle->DiffersFromDefault();
}

void FWxDataTableRowHandleCustomization::HandleResetToDefault()
{
	if (PropertyHandle.IsValid() && PropertyHandle->IsValidHandle())
	{
		PropertyHandle->ResetToDefault();
		RequestRefresh();
	}
}

FWxDataTableRowHandleCustomization::EWxValueAccess FWxDataTableRowHandleCustomization::GetCurrentValue(const UDataTable*& OutDataTable, FName& OutRowName) const
{
	if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
	{
		return EWxValueAccess::Fail;
	}

	TArray<const void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	if (RawData.IsEmpty() || !RawData[0])
	{
		return EWxValueAccess::Fail;
	}

	const FDataTableRowHandle* FirstValue = static_cast<const FDataTableRowHandle*>(RawData[0]);
	for (const void* Raw : RawData)
	{
		if (!Raw)
		{
			return EWxValueAccess::Fail;
		}

		const FDataTableRowHandle* Value = static_cast<const FDataTableRowHandle*>(Raw);
		if (Value->DataTable != FirstValue->DataTable || Value->RowName != FirstValue->RowName)
		{
			return EWxValueAccess::MultipleValues;
		}
	}

	OutDataTable = FirstValue->DataTable;
	OutRowName = FirstValue->RowName;
	return EWxValueAccess::Success;
}

bool FWxDataTableRowHandleCustomization::GetCommonDataTable(const UDataTable*& OutDataTable) const
{
	if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
	{
		return false;
	}

	TArray<const void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	if (RawData.IsEmpty() || !RawData[0])
	{
		return false;
	}

	const FDataTableRowHandle* FirstValue = static_cast<const FDataTableRowHandle*>(RawData[0]);
	for (const void* Raw : RawData)
	{
		if (!Raw || static_cast<const FDataTableRowHandle*>(Raw)->DataTable != FirstValue->DataTable)
		{
			return false;
		}
	}

	OutDataTable = FirstValue->DataTable;
	return true;
}

void FWxDataTableRowHandleCustomization::RequestRefresh() const
{
	if (const TSharedPtr<IPropertyUtilities> Utilities = PropertyUtilities.Pin())
	{
		Utilities->RequestForceRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
