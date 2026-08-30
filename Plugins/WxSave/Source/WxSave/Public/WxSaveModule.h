// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class AActor;
class FProperty;
class UObject;

DECLARE_LOG_CATEGORY_EXTERN(LogWxSave, Log, All);

class FWxSaveModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static bool HandleShouldPersistSceneComponentProperty(const UObject* Object, const FProperty* Property);
	static void HandlePostRestoreObject(const UObject* Object, const TArray<const FProperty*>& RestoredProperties);
	static void HandlePostRestoreSceneComponent(const UObject* Object, const TArray<const FProperty*>& RestoredProperties);
	static bool HandleShouldPersistRuntimeActor(const AActor* Actor);
};
