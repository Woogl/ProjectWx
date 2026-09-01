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
#include "WxStateTreeComponentNameCustomization.h"
#include "WxDeviceLinkVisualizer.h"
#include "WxItemDefinitionThumbnailRenderer.h"
#include "WxUIDataThumbnailRenderer.h"
#include "Device/WxDeviceComponentName.h"

IMPLEMENT_MODULE(FWxEditorModule, WxEditor)

namespace WxEditorModule
{
	static const FName PropertyEditorModuleName(TEXT("PropertyEditor"));
}

void FWxEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);

	// 액터 지정(AllowedLocators="Actor" 메타) UOL 필드에만 한 줄 픽커를 적용한다. 메타 없는 UOL 은 엔진 기본 편집기 유지.
	ActorLocatorIdentifier = MakeShared<FWxActorLocatorTypeIdentifier>();
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FUniversalObjectLocator::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxActorLocatorCustomization::MakeInstance),
		ActorLocatorIdentifier);

	PropertyModule.RegisterCustomPropertyTypeLayout(
		FWxStateTreeComponentName::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxStateTreeComponentNameCustomization::MakeInstance));

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
		PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("WxStateTreeComponentName"));
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
