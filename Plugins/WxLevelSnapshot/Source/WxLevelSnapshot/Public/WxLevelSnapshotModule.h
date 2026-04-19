// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

WXLEVELSNAPSHOT_API DECLARE_LOG_CATEGORY_EXTERN(LogWxLevelSnapshot, Log, All);

class UPackage;
class FObjectPostSaveContext;
struct FAssetData;

/**
 * 레벨 저장 시 그 안의 액터들을 JSON으로 추출하는 에디터 모듈.
 * ExternalActor 패키지(액터 하나)나 일반 레벨 패키지(전체 액터) 저장 양쪽 모두를 처리한다.
 */
class FWxLevelSnapshotModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void HandlePackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context);
	void HandleAssetRemoved(const FAssetData& AssetData);

	FDelegateHandle PackageSavedHandle;
	FDelegateHandle AssetRemovedHandle;
};
