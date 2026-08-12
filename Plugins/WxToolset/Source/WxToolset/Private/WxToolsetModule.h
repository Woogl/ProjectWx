// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWxToolset, Log, All);

class FWxToolsetModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;
};
