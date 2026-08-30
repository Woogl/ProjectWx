// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeComponentNameCustomization.h"

#include "Components/SceneComponent.h"
#include "DetailWidgetRow.h"
#include "GameFramework/Actor.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "StateTreeEditorData.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeSchema.h"
#include "Device/WxDeviceComponentName.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "WxStateTreeComponentNameCustomization"

namespace WxStateTreeComponentNameCustomization
{
	static const FName AllowedClassesName(TEXT("AllowedClasses"));
}

TSharedRef<IPropertyTypeCustomization> FWxStateTreeComponentNameCustomization::MakeInstance()
{
	return MakeShared<FWxStateTreeComponentNameCustomization>();
}

void FWxStateTreeComponentNameCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	using namespace WxStateTreeComponentNameCustomization;

	AllowedComponentClass = USceneComponent::StaticClass();
	if (InPropertyHandle->HasMetaData(AllowedClassesName))
	{
		if (UClass* ResolvedClass = FSoftClassPath(InPropertyHandle->GetMetaData(AllowedClassesName)).ResolveClass())
		{
			AllowedComponentClass = ResolvedClass;
		}
	}

	NamePropertyHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWxStateTreeComponentName, Name));
	if (!NamePropertyHandle.IsValid() || !NamePropertyHandle->IsValidHandle())
	{
		return;
	}

	FPropertyComboBoxArgs ComboArgs(
		NamePropertyHandle,
		FOnGetPropertyComboBoxStrings::CreateSP(this, &FWxStateTreeComponentNameCustomization::HandleGetComponentStrings),
		FOnGetPropertyComboBoxValue::CreateSP(this, &FWxStateTreeComponentNameCustomization::HandleGetComponentValueString));
	ComboArgs.ShowSearchForItemCount = 1;

	HeaderRow
	.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		PropertyCustomizationHelpers::MakePropertyComboBox(ComboArgs)
	];
}

void FWxStateTreeComponentNameCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// 자식을 비워야 확장 화살표가 사라져 접힘 자체가 없어진다 — 이 필드는 한 줄이 전부다.
}

void FWxStateTreeComponentNameCustomization::HandleGetComponentStrings(TArray<TSharedPtr<FString>>& OutStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) const
{
	// 비우는 선택지 — 부착 대상처럼 지정하지 않는 것이 뜻을 갖는 자리가 있다.
	OutStrings.Add(MakeShared<FString>(FName(NAME_None).ToString()));
	OutRestrictedItems.Add(false);

	const UClass* ActorClass = FindContextActorClass();
	if (!ActorClass)
	{
		return;
	}

	TArray<FName> ComponentNames;
	for (TFieldIterator<FObjectPropertyBase> PropertyIt(ActorClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		if (PropertyIt->PropertyClass && PropertyIt->PropertyClass->IsChildOf(AllowedComponentClass))
		{
			ComponentNames.Add(PropertyIt->GetFName());
		}
	}
	ComponentNames.Sort(FNameLexicalLess());

	for (const FName& ComponentName : ComponentNames)
	{
		OutStrings.Add(MakeShared<FString>(ComponentName.ToString()));
		OutRestrictedItems.Add(false);
	}
}

FString FWxStateTreeComponentNameCustomization::HandleGetComponentValueString() const
{
	if (!NamePropertyHandle.IsValid() || !NamePropertyHandle->IsValidHandle())
	{
		return FString();
	}

	FName ComponentName;
	if (NamePropertyHandle->GetValue(ComponentName) == FPropertyAccess::MultipleValues)
	{
		return LOCTEXT("MultipleValues", "Multiple Values").ToString();
	}

	// 후보에 없는 값(컴포넌트가 지워지거나 이름이 바뀐 뒤)도 그대로 보여야 무엇이 어긋났는지 알 수 있다.
	return ComponentName.ToString();
}

const UClass* FWxStateTreeComponentNameCustomization::FindContextActorClass() const
{
	TArray<UObject*> OuterObjects;
	NamePropertyHandle->GetOuterObjects(OuterObjects);

	for (const UObject* OuterObject : OuterObjects)
	{
		if (!OuterObject)
		{
			continue;
		}

		// 태스크 인스턴스 데이터는 상태 오브젝트 안에 있으므로 바깥으로 거슬러 에셋의 편집 데이터를 찾는다.
		const UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(OuterObject);
		if (!EditorData)
		{
			EditorData = OuterObject->GetTypedOuter<UStateTreeEditorData>();
		}

		if (!EditorData || !EditorData->Schema)
		{
			continue;
		}

		for (const FStateTreeExternalDataDesc& ContextDesc : EditorData->Schema->GetContextDataDescs())
		{
			const UClass* ContextClass = Cast<UClass>(ContextDesc.Struct);
			if (ContextClass && ContextClass->IsChildOf(AActor::StaticClass()))
			{
				return ContextClass;
			}
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
