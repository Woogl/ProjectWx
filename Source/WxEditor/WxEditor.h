// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class IPropertyTypeIdentifier;

class FWxEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	/** PIE 시작 델리게이트 핸들러: Experience 매니저의 세션별 GameFeature 활성 카운터를 리셋한다. */
	void HandleBeginPIE(bool bIsSimulating);

	FDelegateHandle BeginPIEHandle;

	/** 액터 지정 UOL 픽커의 조건부 등록 식별자. 해제 시 같은 인스턴스를 넘겨야 하므로 들고 있는다. */
	TSharedPtr<IPropertyTypeIdentifier> ActorLocatorIdentifier;
};
