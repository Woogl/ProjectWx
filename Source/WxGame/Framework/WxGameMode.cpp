// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Engine/GameInstance.h"
#include "Framework/WxGameState.h"
#include "FrontEnd/WxGameFlowSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Player/WxPlayerState.h"

AWxGameMode::AWxGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AWxGameState::StaticClass();
	PlayerStateClass = AWxPlayerState::StaticClass();
}

UClass* AWxGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// FrontEnd 에서 고른 캐릭터가 있으면 그 폰이 DefaultPawnClass 보다 앞선다.
	if (UWxGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UWxGameFlowSubsystem>())
	{
		if (UClass* SelectedClass = Flow->GetSelectedPawnClass(GetWorld()))
		{
			return SelectedClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AWxGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UWxGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UWxGameFlowSubsystem>();
	if (Flow && !Flow->ValidateArrival(GetWorld()))
	{
		return;
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (Flow)
	{
		Flow->HoldArrivalPawn(NewPlayer);
	}

	// 아이템 추가는 BeginPlay 에 기대지 않고 복제 준비 시 기존 엔트리를 back-fill 하므로, 월드 BeginPlay 전에 스폰되는 최초 로컬 플레이어에도 안전하다.
	UWxInventoryComponent* Inventory = NewPlayer ? NewPlayer->FindComponentByClass<UWxInventoryComponent>() : nullptr;
	if (Inventory && !StartingItems.IsEmpty())
	{
		Inventory->GrantItems(StartingItems);
	}
}
