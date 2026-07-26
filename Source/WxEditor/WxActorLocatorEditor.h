// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UniversalObjectLocatorEditor.h"

class AActor;

/**
 * 엔진 스톡 액터 로케이터 에디터를 대체하는 UOL 액터 픽커.
 * 동작은 스톡과 동일하되, 편집 대상 프로퍼티에 AllowedClasses 메타(콤마 구분 클래스 경로)가 있으면 픽커를 해당 클래스(IsA)로 제한한다.
 * 스톡 구현(FActorLocatorEditor)이 엔진 Private 라 서브클래스 대신 동작을 재현한다.
 * "Actor" 이름으로 교체 등록해야 한다 — 현재 값의 표시 에디터가 프래그먼트 타입의 PrimaryEditorType("Actor")으로 조회되기 때문.
 * 아웃라이너 드래그드롭 경로는 API 에 프로퍼티 정보가 없어 필터하지 못한다(잘못 놓으면 노드 표시·런타임 경고로 드러남).
 */
class FWxActorLocatorEditor : public UE::UniversalObjectLocator::ILocatorFragmentEditor
{
public:
	virtual UE::UniversalObjectLocator::ELocatorFragmentEditorType GetLocatorFragmentEditorType() const override;
	virtual bool IsDragSupported(TSharedPtr<FDragDropOperation> InDragOperation, UObject* InContext) const override;
	virtual UObject* ResolveDragOperation(TSharedPtr<FDragDropOperation> InDragOperation, UObject* InContext) const override;
	virtual TSharedPtr<SWidget> MakeEditUI(const FEditUIParameters& InParameters) override;
	virtual FText GetDisplayText(const FUniversalObjectLocatorFragment* InFragment) const override;
	virtual FText GetDisplayTooltip(const FUniversalObjectLocatorFragment* InFragment) const override;
	virtual FSlateIcon GetDisplayIcon(const FUniversalObjectLocatorFragment* InFragment) const override;
	virtual UClass* ResolveClass(const FUniversalObjectLocatorFragment& InFragment, UObject* InContext) const override;
	virtual FUniversalObjectLocatorFragment MakeDefaultLocatorFragment() const override;

private:
	/** 픽커 필터. 허용 클래스 목록이 비면 전체 허용(스톡 동작), 아니면 IsA 일치만 통과한다. */
	bool HandleShouldFilterActor(const AActor* InActor, TArray<const UClass*> AllowedClasses) const;

	/** 픽커 선택 콜백. 선택한 액터로 액터 프래그먼트를 만들어 핸들에 쓴다. */
	void HandleActorSelected(AActor* InActor, TWeakPtr<UE::UniversalObjectLocator::IFragmentEditorHandle> InWeakHandle);
};
