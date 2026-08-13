// Copyright Woogle. All Rights Reserved.

#include "WxEditor.h"

#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "Framework/WxExperienceManager.h"
#include "Items/WxItemDefinition.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UniversalObjectLocator.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "WxAbilityThumbnailRenderer.h"
#include "WxActorLocatorCustomization.h"
#include "WxCategoryDetailCustomization.h"
#include "WxDataTableRowHandleCustomization.h"
#include "WxItemDefinitionThumbnailRenderer.h"

IMPLEMENT_MODULE(FWxEditorModule, WxEditor)

namespace WxEditorModule
{
	static const FName PropertyEditorModuleName(TEXT("PropertyEditor"));
}

void FWxEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);

	PropertyModule.RegisterCustomClassLayout(
		UObject::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FWxCategoryDetailCustomization::MakeInstance));

	// 액터 지정(AllowedLocators="Actor" 메타) UOL 필드에만 한 줄 픽커를 적용한다. 메타 없는 UOL 은 엔진 기본 편집기 유지.
	ActorLocatorIdentifier = MakeShared<FWxActorLocatorTypeIdentifier>();
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FUniversalObjectLocator::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxActorLocatorCustomization::MakeInstance),
		ActorLocatorIdentifier);

	// 식별자 없이 등록해 엔진 기본 레이아웃을 대체한다 — 그쪽은 편집 위젯을 자식 행에 두어 파라미터 백 패널에서 접으면 빈칸이 된다.
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FDataTableRowHandle::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxDataTableRowHandleCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();

	// UWxItemDefinition 의 에디터 썸네일을 아이템 아이콘으로 렌더링한다.
	UThumbnailManager::Get().RegisterCustomRenderer(
		UWxItemDefinition::StaticClass(),
		UWxItemDefinitionThumbnailRenderer::StaticClass());

	// 어빌리티 BP 썸네일을 UIData 아이콘으로 렌더링하기 위해, 엔진이 ini 로 등록한 기본 Blueprint 렌더러를 파생 렌더러로 교체한다.
	// RegisterCustomRenderer 는 동일 클래스 중복 등록을 거부하므로 기존 등록을 먼저 해제해야 한다.
	UThumbnailManager::Get().UnregisterCustomRenderer(UBlueprint::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(
		UBlueprint::StaticClass(),
		UWxAbilityThumbnailRenderer::StaticClass());

	BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FWxEditorModule::HandleBeginPIE);
}

void FWxEditorModule::ShutdownModule()
{
	FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	BeginPIEHandle.Reset();

	if (FModuleManager::Get().IsModuleLoaded(WxEditorModule::PropertyEditorModuleName))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);
		PropertyModule.UnregisterCustomClassLayout(UObject::StaticClass()->GetFName());
		if (ActorLocatorIdentifier.IsValid())
		{
			PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("UniversalObjectLocator"), ActorLocatorIdentifier);
			ActorLocatorIdentifier.Reset();
		}
		PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("DataTableRowHandle"));
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	// UObject 시스템이 아직 살아있을 때만 썸네일 렌더러 등록을 해제한다.
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UWxItemDefinition::StaticClass());

		// 가로챘던 Blueprint 렌더러를 엔진 기본 렌더러로 복원한다.
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
