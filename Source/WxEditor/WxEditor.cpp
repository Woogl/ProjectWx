// Copyright Woogle. All Rights Reserved.

#include "WxEditor.h"

#include "Items/WxItemDefinition.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
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
	}
}
