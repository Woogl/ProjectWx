// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "PropertyEditorDelegates.h"

class IPropertyTypeIdentifier;

class FWxEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	/** Experience 매니저의 세션별 GameFeature 활성 카운터를 리셋한다. */
	void HandleBeginPIE(bool bIsSimulating);

	FDelegateHandle BeginPIEHandle;

	/** 액터 지정 UOL 픽커의 조건부 등록 식별자. 해제 시 같은 인스턴스를 넘겨야 하므로 들고 있는다. */
	TSharedPtr<IPropertyTypeIdentifier> ActorLocatorIdentifier;

	/** "Object" 자리에서 대체한 엔진 원본 커스터마이제이션 콜백. 종료 시 그대로 되돌려 놓는다. */
	FDetailLayoutCallback EngineObjectLayout{};
};
