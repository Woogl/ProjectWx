// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataTableEditorUtils.h"

class UDataTable;
class UScriptStruct;
struct FDataTableRowHandle;

/**
 * 데이터 테이블 행 이름이 바뀌면 그 행을 가리키던 FDataTableRowHandle 을 새 이름으로 고친다. 고친 에셋은 Dirty 로만 두고 저장은 사용자가 한다.
 * 엔진에 행 이름 변경 이벤트가 없어 행 목록 변경 전후의 행 맵을 비교해 알아낸다.
 */
class FDataTableRowReferenceUpdater : public FDataTableEditorUtils::INotifyOnDataTableChanged
{
public:
	virtual void PreChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;
	virtual void PostChange(const UDataTable* Changed, FDataTableEditorUtils::EDataTableChangeInfo Info) override;

private:
	void UpdateReferences(UDataTable* DataTable, FName OldName, FName NewName);

	/** Value(ArrayDim 전체) 안에서 OldRow 와 같은 핸들을 찾는다. bApply 면 찾은 핸들의 행 이름을 NewName 으로 바꾼다. */
	static bool ReplaceRowName(const FProperty* Property, void* Value, const FDataTableRowHandle& OldRow, FName NewName, bool bApply);
	static bool ReplaceRowNameInStruct(const UScriptStruct* Struct, void* Memory, const FDataTableRowHandle& OldRow, FName NewName, bool bApply);

	TWeakObjectPtr<const UDataTable> SnapshotTable;
	TMap<FName, uint8*> SnapshotRows;
};
