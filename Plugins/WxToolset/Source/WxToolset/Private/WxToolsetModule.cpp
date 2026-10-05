// Copyright Woogle. All Rights Reserved.

#include "WxToolsetModule.h"

#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "WxAnimMontageToolset.h"
#include "WxBlueprintToolset.h"
#include "WxLandscapeToolset.h"
#include "WxMVVMToolset.h"
#include "WxPackageToolset.h"
#include "WxStateTreeToolset.h"
#include "WxWaterToolset.h"

DEFINE_LOG_CATEGORY(LogWxToolset);

void FWxToolsetModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UWxAnimMontageToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxBlueprintToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxLandscapeToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxMVVMToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxPackageToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxStateTreeToolset::StaticClass());
	UToolsetRegistry::RegisterToolsetClass(UWxWaterToolset::StaticClass());
}

void FWxToolsetModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UWxAnimMontageToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxBlueprintToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxLandscapeToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxMVVMToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxPackageToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxStateTreeToolset::StaticClass());
	UToolsetRegistry::UnregisterToolsetClass(UWxWaterToolset::StaticClass());
}

IMPLEMENT_MODULE(FWxToolsetModule, WxToolset)
