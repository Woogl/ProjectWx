// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Cheat/WxCheatManager.h"
#include "Components/GameFrameworkComponentManager.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CheatClass = UWxCheatManager::StaticClass();
}

void AWxPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}
