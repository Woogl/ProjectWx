// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FWxEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	/** PIE 시작 델리게이트 핸들러: Experience 매니저의 세션별 GameFeature 활성 카운터를 리셋한다. */
	void HandleBeginPIE(bool bIsSimulating);

	FDelegateHandle BeginPIEHandle;
};
