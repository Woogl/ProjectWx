// Copyright Woogle. All Rights Reserved.

#include "WxToolsetModule.h"

#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "WxStateTreeToolset.h"

DEFINE_LOG_CATEGORY(LogWxToolset);

void FWxToolsetModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UWxStateTreeToolset::StaticClass());
}

void FWxToolsetModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UWxStateTreeToolset::StaticClass());
}

IMPLEMENT_MODULE(FWxToolsetModule, WxToolset)
