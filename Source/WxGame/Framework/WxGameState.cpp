// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Framework/WxExperienceManagerComponent.h"

AWxGameState::AWxGameState()
{
	ExperienceManagerComponent = CreateDefaultSubobject<UWxExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
}

void AWxGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}

UWxExperienceManagerComponent* AWxGameState::GetExperienceManagerComponent() const
{
	return ExperienceManagerComponent;
}
