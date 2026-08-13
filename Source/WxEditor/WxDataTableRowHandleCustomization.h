// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SToolTip;
class UDataTable;
class UScriptStruct;
struct FAssetData;

// 데이터 테이블 행 지정 필드의 한 줄 편집기 — 테이블 픽커와 행 이름 콤보를 헤더 행에 놓고 자식은 만들지 않는다.
// 엔진 기본 레이아웃(FDataTableCustomizationLayout)을 식별자 없이 덮어써 프로젝트 전역에서 대체한다.
// 대체하는 이유: 엔진은 헤더를 비우고 편집 위젯을 자식 행에 두는데, 파라미터 백을 그리는 쪽(FPropertyBagInstanceDataDetails::OnChildRowAdded)은 이름·값 칸을 자기 위젯으로 재구성하며 값 칸에 헤더의 기본 값 위젯을 요구한다.
// 그래서 StateTree 파라미터·링크 상태 오버라이드에서는 접힌 행이 빈칸이 되고 편집 위젯은 자식 안에 숨는다 — 자식을 없애면 확장 화살표째 사라져 항상 한 줄이 된다.
// 일반 디테일 패널은 헤더가 비면 자식을 인라인으로 승격시켜 주던 자리라 보이는 결과가 달라지지 않는다.
// CustomizeHeader 는 같은 행에서 두 번 돌 수 있으므로(엔진이 버려질 행에 한 번 더 실행) 호출마다 핸들과 위젯을 다시 만든다.
class FWxDataTableRowHandleCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	void HandleDataTableChanged();
	bool HandleShouldFilterAsset(const FAssetData& AssetData);
	void HandleGetRowStrings(TArray<TSharedPtr<FString>>& OutStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const;
	FString HandleGetRowValueString() const;
	void HandleSearchForReferences();

	/** 두 핸들을 함께 읽는다. 어느 쪽이든 다중 값이면 실패다. */
	bool GetCurrentValue(UDataTable*& OutDataTable, FName& OutRowName) const;

	TSharedPtr<IPropertyHandle> DataTablePropertyHandle;

	TSharedPtr<IPropertyHandle> RowNamePropertyHandle;

	/** RowType 메타의 행 구조체. 비면 테이블 후보를 거르지 않는다. 네이티브 구조체만 상정하므로 GC 소유 표기가 필요 없다. */
	FName RowTypeFilter;
	UScriptStruct* RowFilterStruct = nullptr;
};
