// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDataTableRowFixup, Log, All);

class FDataTableRowReferenceUpdater;

class FDataTableRowFixupModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	TUniquePtr<FDataTableRowReferenceUpdater> ReferenceUpdater;
};
