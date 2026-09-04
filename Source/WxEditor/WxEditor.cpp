// Copyright Woogle. All Rights Reserved.

#include "WxEditor.h"

#include "Components/StateTreeComponent.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "Framework/WxExperienceManager.h"
#include "Items/WxItemDefinition.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UniversalObjectLocator.h"
#include "UnrealEdGlobals.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "WxActorLocatorCustomization.h"
#include "WxDataTableRowHandleCustomization.h"
#include "WxStateTreeComponentNameCustomization.h"
#include "WxDeviceLinkVisualizer.h"
#include "WxItemDefinitionThumbnailRenderer.h"
#include "WxObjectDetails.h"
#include "WxUIDataThumbnailRenderer.h"
#include "Device/WxDeviceComponentName.h"

IMPLEMENT_MODULE(FWxEditorModule, WxEditor)

namespace WxEditorModule
{
	static const FName PropertyEditorModuleName(TEXT("PropertyEditor"));
	static const FName DetailCustomizationsModuleName(TEXT("DetailCustomizations"));
	/** 엔진이 FObjectDetails 를 등록할 때 쓰는 클래스 이름 그대로. */
	static const FName ObjectClassName(TEXT("Object"));
	static const FName WxSectionName(TEXT("Wx"));
}

void FWxEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);

	ActorLocatorIdentifier = MakeShared<FWxActorLocatorTypeIdentifier>();
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FUniversalObjectLocator::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxActorLocatorCustomization::MakeInstance),
		ActorLocatorIdentifier);

	DataTableRowHandleIdentifier = MakeShared<FWxDataTableRowHandleTypeIdentifier>();
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FDataTableRowHandle::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxDataTableRowHandleCustomization::MakeInstance),
		DataTableRowHandleIdentifier);

	PropertyModule.RegisterCustomPropertyTypeLayout(
		FWxStateTreeComponentName::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxStateTreeComponentNameCustomization::MakeInstance));

	// "Object" 는 엔진 DetailCustomizations 가 FObjectDetails 로 차지한 자리고 재등록은 교체다. 원본을 꺼내 안에 품고 실행 순서(Order)도 그대로 잇는다.
	FModuleManager::Get().LoadModuleChecked(WxEditorModule::DetailCustomizationsModuleName);
	FRegisterCustomClassLayoutParams ObjectLayoutParams;
	if (const FDetailLayoutCallback* EngineCallback = PropertyModule.GetClassNameToDetailLayoutNameMap().Find(WxEditorModule::ObjectClassName))
	{
		EngineObjectLayout = *EngineCallback;
		ObjectLayoutParams.OptionalOrder = EngineCallback->Order;
	}
	PropertyModule.RegisterCustomClassLayout(
		WxEditorModule::ObjectClassName,
		FOnGetDetailCustomizationInstance::CreateStatic(&FWxObjectDetails::MakeInstance, EngineObjectLayout.DetailLayoutDelegate),
		ObjectLayoutParams);

	// 레벨 에디터 디테일의 섹션 탭. 엔진 섹션은 전부 기본 Order(0)라 알파벳순이므로, 더 작은 값으로 General 바로 다음에 둔다.
	PropertyModule.FindOrCreateSection(WxEditorModule::ObjectClassName, WxEditorModule::WxSectionName, INVTEXT("Wx"), -1)
		->AddCategory(FWxObjectDetails::WxCategoryName);

	PropertyModule.NotifyCustomizationModuleChanged();

	UThumbnailManager::Get().RegisterCustomRenderer(
		UWxItemDefinition::StaticClass(),
		UWxItemDefinitionThumbnailRenderer::StaticClass());

	// 어빌리티·GE BP 썸네일을 각자의 테이블 행 아이콘으로 렌더링하기 위해, 엔진이 ini 로 등록한 기본 Blueprint 렌더러를 파생 렌더러로 교체한다.
	// RegisterCustomRenderer 는 동일 클래스 중복 등록을 거부하므로 기존 등록을 먼저 해제해야 한다.
	UThumbnailManager::Get().UnregisterCustomRenderer(UBlueprint::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(
		UBlueprint::StaticClass(),
		UWxUIDataThumbnailRenderer::StaticClass());

	BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FWxEditorModule::HandleBeginPIE);

	// 엔진은 UStateTreeComponent 자리를 비워 두었고, 에디터가 클래스 사슬을 거슬러 찾으므로 장치의 파생 컴포넌트까지 덮인다.
	if (GUnrealEd)
	{
		GUnrealEd->RegisterComponentVisualizer(UStateTreeComponent::StaticClass()->GetFName(), MakeShared<FWxDeviceLinkVisualizer>());
	}
}

void FWxEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UStateTreeComponent::StaticClass()->GetFName());
	}

	FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	BeginPIEHandle.Reset();

	if (FModuleManager::Get().IsModuleLoaded(WxEditorModule::PropertyEditorModuleName))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);
		if (ActorLocatorIdentifier.IsValid())
		{
			PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("UniversalObjectLocator"), ActorLocatorIdentifier);
			ActorLocatorIdentifier.Reset();
		}
		if (DataTableRowHandleIdentifier.IsValid())
		{
			PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("DataTableRowHandle"), DataTableRowHandleIdentifier);
			DataTableRowHandleIdentifier.Reset();
		}
		PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("WxStateTreeComponentName"));

		if (EngineObjectLayout.DetailLayoutDelegate.IsBound())
		{
			FRegisterCustomClassLayoutParams ObjectLayoutParams;
			ObjectLayoutParams.OptionalOrder = EngineObjectLayout.Order;
			PropertyModule.RegisterCustomClassLayout(WxEditorModule::ObjectClassName, EngineObjectLayout.DetailLayoutDelegate, ObjectLayoutParams);
		}
		else
		{
			PropertyModule.UnregisterCustomClassLayout(WxEditorModule::ObjectClassName);
		}
		PropertyModule.RemoveSection(WxEditorModule::ObjectClassName, WxEditorModule::WxSectionName);

		PropertyModule.NotifyCustomizationModuleChanged();
	}

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UWxItemDefinition::StaticClass());

		UThumbnailManager::Get().UnregisterCustomRenderer(UBlueprint::StaticClass());
		UThumbnailManager::Get().RegisterCustomRenderer(
			UBlueprint::StaticClass(),
			UBlueprintThumbnailRenderer::StaticClass());
	}
}

void FWxEditorModule::HandleBeginPIE(bool bIsSimulating)
{
	UWxExperienceManager* ExperienceManager = GEngine->GetEngineSubsystem<UWxExperienceManager>();
	check(ExperienceManager);

	ExperienceManager->OnPlayInEditorBegun();
}
