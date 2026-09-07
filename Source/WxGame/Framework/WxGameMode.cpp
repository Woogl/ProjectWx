// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Engine/GameInstance.h"
#include "Framework/WxGameState.h"
#include "FrontEnd/WxGameFlowSubsystem.h"
#include "Player/WxPlayerState.h"

AWxGameMode::AWxGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AWxGameState::StaticClass();
	PlayerStateClass = AWxPlayerState::StaticClass();
}

UClass* AWxGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (UWxGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UWxGameFlowSubsystem>())
	{
		if (UClass* SelectedClass = Flow->GetSelectedPawnClass(GetWorld()))
		{
			return SelectedClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}
