// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

class FBoxComponentVisualizerEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;
};
