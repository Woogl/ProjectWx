// Copyright Woogle. All Rights Reserved.

#include "WxEditor.h"

#include "Engine/Blueprint.h"
#include "IUniversalObjectLocatorEditorModule.h"
#include "Items/WxItemDefinition.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "WxAbilityThumbnailRenderer.h"
#include "WxActorLocatorEditor.h"
#include "WxCategoryDetailCustomization.h"
#include "WxItemDefinitionThumbnailRenderer.h"

IMPLEMENT_MODULE(FWxEditorModule, WxEditor)

namespace WxEditorModule
{
	static const FName PropertyEditorModuleName(TEXT("PropertyEditor"));
	static const FName UolEditorModuleName(TEXT("UniversalObjectLocatorEditor"));
	static const FName ActorLocatorEditorName(TEXT("Actor"));
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

	// UOL 액터 픽커를 AllowedClasses 메타를 읽는 교체 에디터로 바꾼다.
	// 스톡 "Actor" 등록이 선행되도록 UOL 에디터 모듈을 먼저 로드한 뒤 같은 이름으로 덮어쓴다 — 메타 없는 용처는 동작이 동일하다.
	UE::UniversalObjectLocator::IUniversalObjectLocatorEditorModule& UolEditorModule =
		FModuleManager::LoadModuleChecked<UE::UniversalObjectLocator::IUniversalObjectLocatorEditorModule>(WxEditorModule::UolEditorModuleName);
	UolEditorModule.RegisterLocatorEditor(WxEditorModule::ActorLocatorEditorName, MakeShared<FWxActorLocatorEditor>());
}

void FWxEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(WxEditorModule::PropertyEditorModuleName))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);
		PropertyModule.UnregisterCustomClassLayout(UObject::StaticClass()->GetFName());
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	// 우리 로케이터 에디터 인스턴스가 모듈 언로드 후까지 등록부에 남지 않게 해제한다(스톡 복원은 비공개라 불가 — 종료 경로라 무해).
	if (FModuleManager::Get().IsModuleLoaded(WxEditorModule::UolEditorModuleName))
	{
		UE::UniversalObjectLocator::IUniversalObjectLocatorEditorModule& UolEditorModule =
			FModuleManager::GetModuleChecked<UE::UniversalObjectLocator::IUniversalObjectLocatorEditorModule>(WxEditorModule::UolEditorModuleName);
		UolEditorModule.UnregisterLocatorEditor(WxEditorModule::ActorLocatorEditorName);
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
