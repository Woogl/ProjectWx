// Copyright Woogle. All Rights Reserved.

#include "DataTableRowFixupModule.h"

#include "DataTableRowReferenceUpdater.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogDataTableRowFixup);

void FDataTableRowFixupModule::StartupModule()
{
	ReferenceUpdater = MakeUnique<FDataTableRowReferenceUpdater>();
}

void FDataTableRowFixupModule::ShutdownModule()
{
	ReferenceUpdater.Reset();
}

IMPLEMENT_MODULE(FDataTableRowFixupModule, DataTableRowFixup)
