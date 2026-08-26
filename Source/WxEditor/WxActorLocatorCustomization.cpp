// Copyright Woogle. All Rights Reserved.

#include "WxActorLocatorCustomization.h"

#include "DetailWidgetRow.h"
#include "Editor.h"
#include "GameFramework/Actor.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "UniversalObjectLocator.h"
#include "UObject/SoftObjectPath.h"
#include "WxLocatorUtils.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace WxActorLocatorCustomization
{
	static const FName AllowedLocatorsName(TEXT("AllowedLocators"));
	static const FName AllowedClassesName(TEXT("AllowedClasses"));

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

bool FWxActorLocatorTypeIdentifier::IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const
{
	using namespace WxActorLocatorCustomization;

	const FProperty* MetaCarrier = FindMetaCarrier(PropertyHandle.GetProperty(), AllowedLocatorsName);
	return MetaCarrier && MetaCarrier->GetMetaData(AllowedLocatorsName) == TEXT("Actor");
}

TSharedRef<IPropertyTypeCustomization> FWxActorLocatorCustomization::MakeInstance()
{
	return MakeShared<FWxActorLocatorCustomization>();
}

void FWxActorLocatorCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	using namespace WxActorLocatorCustomization;

	PropertyHandle = InPropertyHandle;

	AllowedClass = AActor::StaticClass();
	if (const FProperty* MetaCarrier = FindMetaCarrier(InPropertyHandle->GetProperty(), AllowedClassesName))
	{
		if (UClass* ResolvedClass = FSoftClassPath(MetaCarrier->GetMetaData(AllowedClassesName)).ResolveClass())
		{
			AllowedClass = ResolvedClass;
		}
	}

	HeaderRow
	.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.f)
	.MaxDesiredWidth(600.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SAssignNew(PickerCombo, SComboButton)
			.OnGetMenuContent(this, &FWxActorLocatorCustomization::HandleGetPickerMenu)
			.ContentPadding(2.f)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FWxActorLocatorCustomization::HandleGetCurrentActorText)
				.Font(CustomizationUtils.GetRegularFont())
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f, 0.f, 0.f)
		[
			PropertyCustomizationHelpers::MakeInteractiveActorPicker(
				FOnGetAllowedClasses::CreateSP(this, &FWxActorLocatorCustomization::HandleGetAllowedClasses),
				FOnShouldFilterActor::CreateSP(this, &FWxActorLocatorCustomization::HandleShouldFilterActor),
				FOnActorSelected::CreateSP(this, &FWxActorLocatorCustomization::HandleActorSelected))
		]
	];
}

void FWxActorLocatorCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// 내부 프래그먼트 행은 픽커로 대체되므로 펼치지 않는다.
}

TSharedRef<SWidget> FWxActorLocatorCustomization::HandleGetPickerMenu()
{
	return PropertyCustomizationHelpers::MakeActorPickerWithMenu(
		GetCurrentActor(),
		/*AllowClear*/ true,
		FOnShouldFilterActor::CreateSP(this, &FWxActorLocatorCustomization::HandleShouldFilterActor),
		FOnActorSelected::CreateSP(this, &FWxActorLocatorCustomization::HandleActorSelected),
		FSimpleDelegate::CreateSP(this, &FWxActorLocatorCustomization::HandleCloseMenu),
		FSimpleDelegate::CreateSP(this, &FWxActorLocatorCustomization::HandleUseSelectedActor));
}

FText FWxActorLocatorCustomization::HandleGetCurrentActorText() const
{
	TArray<const void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	if (RawData.IsEmpty() || !RawData[0])
	{
		return NSLOCTEXT("WxActorLocator", "None", "None");
	}

	// 빈 값은 표준 액터 픽커와 같은 None 으로 보이게 한다 — 헬퍼의 unset 은 ST 노드 설명용 문구다.
	const FUniversalObjectLocator* Locator = static_cast<const FUniversalObjectLocator*>(RawData[0]);
	if (Locator->IsEmpty())
	{
		return NSLOCTEXT("WxActorLocator", "None", "None");
	}

	// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
	return FText::FromString(FWxLocatorUtils::GetDisplayName(*Locator));
}

bool FWxActorLocatorCustomization::HandleShouldFilterActor(const AActor* Actor) const
{
	return Actor && Actor->IsA(AllowedClass);
}

void FWxActorLocatorCustomization::HandleActorSelected(AActor* Actor)
{
	SetLocatorToActor(Actor);
}

void FWxActorLocatorCustomization::HandleGetAllowedClasses(TArray<const UClass*>& OutClasses)
{
	OutClasses.Add(AllowedClass);
}

void FWxActorLocatorCustomization::HandleUseSelectedActor()
{
	if (!GEditor)
	{
		return;
	}

	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		if (Actor && HandleShouldFilterActor(Actor))
		{
			SetLocatorToActor(Actor);
			break;
		}
	}

	HandleCloseMenu();
}

void FWxActorLocatorCustomization::HandleCloseMenu()
{
	if (PickerCombo.IsValid())
	{
		PickerCombo->SetIsOpen(false);
	}
}

AActor* FWxActorLocatorCustomization::GetCurrentActor() const
{
	TArray<const void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	if (RawData.IsEmpty() || !RawData[0])
	{
		return nullptr;
	}

	const FUniversalObjectLocator* Locator = static_cast<const FUniversalObjectLocator*>(RawData[0]);
	return Cast<AActor>(Locator->SyncFind());
}

void FWxActorLocatorCustomization::SetLocatorToActor(AActor* Actor)
{
	const FScopedTransaction Transaction(NSLOCTEXT("WxActorLocator", "SetActor", "Set Actor Locator"));
	PropertyHandle->NotifyPreChange();

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	for (void* Raw : RawData)
	{
		if (!Raw)
		{
			continue;
		}

		FUniversalObjectLocator* Locator = static_cast<FUniversalObjectLocator*>(Raw);
		if (Actor)
		{
			*Locator = FUniversalObjectLocator(Actor);
		}
		else
		{
			Locator->Reset();
		}
	}

	PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	PropertyHandle->NotifyFinishedChangingProperties();
}
