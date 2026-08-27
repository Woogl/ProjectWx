// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;
class SToolTip;
class UClass;

// StateTree 노드의 컴포넌트 지정 필드(FWxStateTreeComponentName) 한 줄 편집기 — 그 트리가 붙을 액터가 가진 컴포넌트 이름 콤보.
// 후보는 ST 에셋 스키마가 정한 Context 액터 클래스의 컴포넌트 프로퍼티에서 뽑으므로 네이티브 컴포넌트와 BP 컴포넌트가 같이 잡힌다.
// 엔진 바인딩 피커를 쓰지 못해 이 편집기가 필요하다 — 그쪽은 소스 프로퍼티에 CPF_Edit 를 요구하고 BP 컴포넌트의 클래스 변수엔 그 플래그가 없다.
// AllowedClasses 메타(클래스 경로 1개)가 있으면 후보를 그 컴포넌트 클래스로 제한한다.
// CustomizeHeader 는 ST 인스턴스 데이터 직속 행에서 두 번 돌 수 있으므로(엔진이 버려질 행에 한 번 더 실행) 호출마다 핸들과 위젯을 다시 만든다.
class FWxStateTreeComponentNameCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	void HandleGetComponentStrings(TArray<TSharedPtr<FString>>& OutStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const;
	FString HandleGetComponentValueString() const;

	/** 이 필드를 담은 ST 에셋 스키마의 Context 액터 클래스. 스키마 밖에서 쓰이거나 지정 전이면 nullptr 이라 후보가 비게 된다. */
	const UClass* FindContextActorClass() const;

	TSharedPtr<IPropertyHandle> NamePropertyHandle;

	/** AllowedClasses 메타가 없으면 USceneComponent. 네이티브 클래스만 상정하므로 GC 소유 표기가 필요 없다. */
	UClass* AllowedComponentClass = nullptr;
};
