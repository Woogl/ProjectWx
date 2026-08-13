// Copyright Woogle. All Rights Reserved.

#include "WxToolsetModule.h"

#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "WxBlueprintToolset.h"
#include "WxStateTreeToolset.h"

DEFINE_LOG_CATEGORY(LogWxToolset);

void FWxToolsetModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UWxBlueprintToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxStateTreeToolset::StaticClass());
}

void FWxToolsetModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UWxBlueprintToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxStateTreeToolset::StaticClass());
}

IMPLEMENT_MODULE(FWxToolsetModule, WxToolset)
