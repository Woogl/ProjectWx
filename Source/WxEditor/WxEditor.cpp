// Copyright Woogle. All Rights Reserved.

#include "WxEditor.h"

#include "Engine/Blueprint.h"
#include "Items/WxItemDefinition.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "WxAbilityThumbnailRenderer.h"
#include "WxCategoryDetailCustomization.h"
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
}

void FWxEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(WxEditorModule::PropertyEditorModuleName))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);
		PropertyModule.UnregisterCustomClassLayout(UObject::StaticClass()->GetFName());
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
