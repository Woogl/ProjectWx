// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Cheat/WxCheatManager.h"
#include "Components/GameFrameworkComponentManager.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 클래스만 지정해 두면 엔진이 Standalone·에디터에서만 실제로 생성한다.
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
