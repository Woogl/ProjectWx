// Copyright Woogle. All Rights Reserved.

#include "DataTableRowReferenceUpdater.h"

#include "AssetRegistry/AssetIdentifier.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "DataTableRowFixupModule.h"
#include "DataTableRowFixupSettings.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/ScopedSlowTask.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Package.h"
#include "UObject/PropertyAccessUtil.h"
#include "UObject/UObjectHash.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DataTableRowReferenceUpdater"

void FDataTableRowReferenceUpdater::PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	if (Info != FDataTableEditorUtils::EDataTableChangeInfo::RowList || !Changed
		|| !GetDefault<UDataTableRowFixupSettings>()->bUpdateReferencesOnRowRename)
	{
		return;
	}

	SnapshotTable = Changed;
	SnapshotRows = Changed->GetRowMap();
}

void FDataTableRowReferenceUpdater::PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info)
{
	if (Info != FDataTableEditorUtils::EDataTableChangeInfo::RowList || !Changed || SnapshotTable.Get() != Changed)
	{
		return;
	}

	const TMap<FName, uint8*> RowsBefore = MoveTemp(SnapshotRows);
	SnapshotTable.Reset();

	const TMap<FName, uint8*>& RowsAfter = Changed->GetRowMap();
	if (RowsBefore.Num() != RowsAfter.Num())
	{
		return;
	}

	// 행 수가 그대로이므로 사라진 이름이 하나면 새로 생긴 이름도 하나다.
	FName OldName;
	for (const TPair<FName, uint8*>& Row : RowsBefore)
	{
		if (RowsAfter.Contains(Row.Key))
		{
			continue;
		}
		if (!OldName.IsNone())
		{
			return;
		}
		OldName = Row.Key;
	}
	if (OldName.IsNone())
	{
		return;
	}

	FName NewName;
	for (const TPair<FName, uint8*>& Row : RowsAfter)
	{
		if (!RowsBefore.Contains(Row.Key))
		{
			NewName = Row.Key;
			break;
		}
	}

	// RenameRow 는 행 메모리를 그대로 새 키로 옮긴다. 재임포트·시트 붙여넣기는 행을 새로 만들므로 이름 변경으로 보지 않는다.
	if (RowsAfter.FindChecked(NewName) != RowsBefore.FindChecked(OldName))
	{
		UE_LOG(LogDataTableRowFixup, Warning, TEXT("%s: '%s' -> '%s' 는 행 데이터가 새로 만들어진 교체라 이름 변경으로 보지 않았습니다. '%s' 참조는 갱신하지 않았습니다."),
			*Changed->GetPathName(), *OldName.ToString(), *NewName.ToString(), *OldName.ToString());
		return;
	}

	UpdateReferences(const_cast<UDataTable*>(Changed), OldName, NewName);
}

void FDataTableRowReferenceUpdater::UpdateReferences(UDataTable* DataTable, FName OldName, FName NewName)
{
	FDataTableRowHandle OldRow;
	OldRow.DataTable = DataTable;
	OldRow.RowName = OldName;

	// 초기 스캔이 끝나지 않았으면 참조처 목록이 모자란다.
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	AssetRegistry.WaitForCompletion();

	// 핸들은 저장될 때 (테이블, 행 이름)을 SearchableName 으로 남긴다. 엔진의 Find Row References 가 쓰는 것과 같은 기록이다.
	TArray<FAssetIdentifier> Referencers;
	AssetRegistry.GetReferencers(FAssetIdentifier(DataTable, OldName), Referencers, UE::AssetRegistry::EDependencyCategory::SearchableName);

	// 레지스트리는 디스크 기준이라 저장하지 않은 변경은 Dirty 패키지에서 찾는다.
	TArray<UPackage*> Packages;
	FEditorFileUtils::GetDirtyPackages(Packages);

	FScopedSlowTask SlowTask(Referencers.Num() + 1, LOCTEXT("UpdatingRowReferences", "Updating data table row references..."));
	SlowTask.MakeDialogDelayed(0.5f);

	int32 ProblemCount = 0;
	TSet<FName> UnresolvedPackageNames;
	for (const FAssetIdentifier& Referencer : Referencers)
	{
		SlowTask.EnterProgressFrame();

		const FString PackageName = Referencer.PackageName.ToString();
		UPackage* Package = FindPackage(nullptr, *PackageName);
		if (!Package)
		{
			// 엔진 에셋 이름 변경처럼 열려 있지 않은 레벨(외부 액터 포함)은 로드하지 않는다.
			if (FEditorFileUtils::IsMapPackageAsset(PackageName))
			{
				UE_LOG(LogDataTableRowFixup, Warning, TEXT("행 참조 갱신 건너뜀(열려 있지 않은 레벨): %s"), *PackageName);
				++ProblemCount;
				continue;
			}
			Package = LoadPackage(nullptr, *PackageName, LOAD_None);
		}

		if (Package)
		{
			Packages.AddUnique(Package);
		}
		UnresolvedPackageNames.Add(Referencer.PackageName);
	}

	SlowTask.EnterProgressFrame();

	TSet<UPackage*> UpdatedPackages;
	TSet<UStateTree*> StateTrees;
	for (UPackage* Package : Packages)
	{
		TArray<UObject*> Objects;
		GetObjectsWithPackage(Package, Objects, EGetObjectsFlags::IncludeNestedObjects, RF_Transient, EInternalObjectFlags::Garbage);
		for (UObject* Object : Objects)
		{
			// 앞서 고친 액터의 구성 스크립트가 다시 돌면서 컴포넌트를 지웠을 수 있다.
			if (!IsValid(Object))
			{
				continue;
			}

			// 행 맵은 리플렉션 밖이라 행 구조체로 직접 순회한다.
			if (UDataTable* Table = Cast<UDataTable>(Object))
			{
				const UScriptStruct* RowStruct = Table->GetRowStruct();
				if (!RowStruct)
				{
					continue;
				}

				bool bFound = false;
				for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
				{
					bFound |= ReplaceRowNameInStruct(RowStruct, Row.Value, OldRow, NewName, false);
				}
				if (!bFound)
				{
					continue;
				}

				FDataTableEditorUtils::BroadcastPreChange(Table, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
				Table->Modify();
				for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
				{
					ReplaceRowNameInStruct(RowStruct, Row.Value, OldRow, NewName, true);
				}
				FDataTableEditorUtils::BroadcastPostChange(Table, FDataTableEditorUtils::EDataTableChangeInfo::RowData);

				UE_LOG(LogDataTableRowFixup, Log, TEXT("행 참조 갱신: %s"), *Table->GetPathName());
				UpdatedPackages.Add(Package);
				UnresolvedPackageNames.Remove(Package->GetFName());
				continue;
			}

			for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
			{
				const FProperty* Property = *It;
				void* Value = Property->ContainerPtrToValuePtr<void>(Object);
				if (!ReplaceRowName(Property, Value, OldRow, NewName, false))
				{
					continue;
				}

				// 파이썬 set_editor_property 와 같은 엔진 경로다. Modify·변경 통지와, 값을 물려받은 로드된 인스턴스로의 전파를 엔진이 처리한다.
				void* NewValue = Property->AllocateAndInitializeValue();
				Property->CopyCompleteValue(NewValue, Value);
				ReplaceRowName(Property, NewValue, OldRow, NewName, true);
				const EPropertyAccessResultFlags Result = PropertyAccessUtil::SetPropertyValue_Object(
					Property, Object, Property, NewValue, INDEX_NONE, 0, EPropertyAccessChangeNotifyMode::Default);
				Property->DestroyAndFreeValue(NewValue);

				UnresolvedPackageNames.Remove(Package->GetFName());
				if (Result != EPropertyAccessResultFlags::Success)
				{
					UE_LOG(LogDataTableRowFixup, Warning, TEXT("행 참조 갱신 실패(편집할 수 없는 프로퍼티): %s.%s"), *Object->GetPathName(), *Property->GetName());
					++ProblemCount;
					continue;
				}

				UE_LOG(LogDataTableRowFixup, Log, TEXT("행 참조 갱신: %s.%s"), *Object->GetPathName(), *Property->GetName());
				UpdatedPackages.Add(Package);
				if (UStateTree* StateTree = Object->GetTypedOuter<UStateTree>())
				{
					StateTrees.Add(StateTree);
				}
			}
		}
	}

	// StateTree 는 에디터 데이터를 컴파일한 결과로 실행되므로 다시 컴파일해야 새 행 이름이 반영된다.
	for (UStateTree* StateTree : StateTrees)
	{
		FStateTreeCompilerLog CompilerLog;
		UStateTreeEditingSubsystem::CompileStateTree(StateTree, CompilerLog);
	}

	for (const FName& PackageName : UnresolvedPackageNames)
	{
		UE_LOG(LogDataTableRowFixup, Warning, TEXT("행 참조 갱신 실패(참조 기록은 있으나 로드하지 못했거나 메모리의 값에 없음): %s"), *PackageName.ToString());
		++ProblemCount;
	}

	if (UpdatedPackages.IsEmpty() && ProblemCount == 0)
	{
		return;
	}

	UE_LOG(LogDataTableRowFixup, Display, TEXT("%s: '%s' -> '%s' 참조를 에셋 %d개에서 갱신했습니다(저장 필요). 갱신하지 못한 참조 %d건."),
		*DataTable->GetPathName(), *OldName.ToString(), *NewName.ToString(), UpdatedPackages.Num(), ProblemCount);

	FText Message = FText::Format(LOCTEXT("RowReferencesUpdated", "Row '{0}' renamed to '{1}': updated references in {2} asset(s). Save them to keep the change."),
		FText::FromName(OldName), FText::FromName(NewName), UpdatedPackages.Num());
	if (ProblemCount > 0)
	{
		Message = FText::Format(LOCTEXT("RowReferencesProblems", "{0}\n{1} reference(s) could not be updated. See Output Log."), Message, ProblemCount);
	}

	FNotificationInfo Info(Message);
	Info.ExpireDuration = 8.f;
	if (const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
	{
		Notification->SetCompletionState(ProblemCount > 0 ? SNotificationItem::CS_Fail : SNotificationItem::CS_Success);
	}
}

bool FDataTableRowReferenceUpdater::ReplaceRowName(const FProperty* Property, void* Value, const FDataTableRowHandle& OldRow, FName NewName, bool bApply)
{
	bool bFound = false;
	for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
	{
		void* Element = static_cast<uint8*>(Value) + Index * Property->GetElementSize();

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == FDataTableRowHandle::StaticStruct())
			{
				FDataTableRowHandle& Handle = *static_cast<FDataTableRowHandle*>(Element);
				if (Handle == OldRow)
				{
					bFound = true;
					if (bApply)
					{
						Handle.RowName = NewName;
					}
				}
			}
			else if (StructProperty->Struct == FInstancedStruct::StaticStruct())
			{
				FInstancedStruct& Instanced = *static_cast<FInstancedStruct*>(Element);
				if (Instanced.IsValid())
				{
					bFound |= ReplaceRowNameInStruct(Instanced.GetScriptStruct(), Instanced.GetMutableMemory(), OldRow, NewName, bApply);
				}
			}
			else
			{
				bFound |= ReplaceRowNameInStruct(StructProperty->Struct, Element, OldRow, NewName, bApply);
			}
		}
		// 컨테이너는 중첩되지 않아 원소가 구조체일 때만 핸들을 담는다. 큰 수치 배열을 원소마다 훑지 않도록 먼저 거른다.
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property); ArrayProperty && ArrayProperty->Inner->IsA<FStructProperty>())
		{
			FScriptArrayHelper Helper(ArrayProperty, Element);
			for (int32 ElementIndex = 0; ElementIndex < Helper.Num(); ++ElementIndex)
			{
				bFound |= ReplaceRowName(ArrayProperty->Inner, Helper.GetRawPtr(ElementIndex), OldRow, NewName, bApply);
			}
		}
		else if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property); SetProperty && SetProperty->ElementProp->IsA<FStructProperty>())
		{
			FScriptSetHelper Helper(SetProperty, Element);
			bool bFoundInElements = false;
			for (FScriptSetHelper::FIterator It(Helper); It; ++It)
			{
				bFoundInElements |= ReplaceRowName(SetProperty->ElementProp, Helper.GetElementPtr(It), OldRow, NewName, bApply);
			}
			// 원소 값이 바뀌면 해시도 바뀐다.
			if (bFoundInElements && bApply)
			{
				Helper.Rehash();
			}
			bFound |= bFoundInElements;
		}
		else if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property);
			MapProperty && (MapProperty->KeyProp->IsA<FStructProperty>() || MapProperty->ValueProp->IsA<FStructProperty>()))
		{
			FScriptMapHelper Helper(MapProperty, Element);
			bool bFoundInKeys = false;
			for (FScriptMapHelper::FIterator It(Helper); It; ++It)
			{
				bFoundInKeys |= ReplaceRowName(MapProperty->KeyProp, Helper.GetKeyPtr(It), OldRow, NewName, bApply);
				bFound |= ReplaceRowName(MapProperty->ValueProp, Helper.GetValuePtr(It), OldRow, NewName, bApply);
			}
			if (bFoundInKeys && bApply)
			{
				Helper.Rehash();
			}
			bFound |= bFoundInKeys;
		}
	}
	return bFound;
}

bool FDataTableRowReferenceUpdater::ReplaceRowNameInStruct(const UScriptStruct* Struct, void* Memory, const FDataTableRowHandle& OldRow, FName NewName, bool bApply)
{
	bool bFound = false;
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		bFound |= ReplaceRowName(*It, It->ContainerPtrToValuePtr<void>(Memory), OldRow, NewName, bApply);
	}
	return bFound;
}

#undef LOCTEXT_NAMESPACE
