// Copyright Woogle. All Rights Reserved.

#include "Player/WxPlayerState.h"

#include "Components/GameFrameworkComponentManager.h"

void AWxPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// ModularGameplay 컴포넌트 수신 opt-in. 활성 주입 요청의 컴포넌트가 여기에 자동 부착된다.
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}
