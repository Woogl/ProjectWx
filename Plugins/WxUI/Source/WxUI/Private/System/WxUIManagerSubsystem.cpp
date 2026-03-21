// Copyright Woogle. All Rights Reserved.

#include "System/WxUIManagerSubsystem.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIDeveloperSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

void UWxUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		HandleLocalPlayerAdded(LocalPlayer);
	}

	GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &ThisClass::HandleLocalPlayerAdded);
}

void UWxUIManagerSubsystem::Deinitialize()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);

		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			if (LocalPlayer)
			{
				LocalPlayer->OnPlayerControllerChanged().RemoveAll(this);
			}
		}
	}

	Super::Deinitialize();
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushContentToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!PrimaryGameLayout || !WidgetClass)
	{
		return nullptr;
	}
	return PrimaryGameLayout->PushWidgetToLayerStack(LayerTag, WidgetClass);
}

UCommonActivatableWidget* UWxUIManagerSubsystem::PushWidgetInstanceToLayer(FGameplayTag LayerTag, UCommonActivatableWidget* WidgetInstance)
{
	if (!PrimaryGameLayout || !WidgetInstance)
	{
		return nullptr;
	}
	return PrimaryGameLayout->PushWidgetInstanceToLayerStack(LayerTag, WidgetInstance);
}

UWxPrimaryGameLayout* UWxUIManagerSubsystem::GetPrimaryGameLayout() const
{
	return PrimaryGameLayout;
}

void UWxUIManagerSubsystem::HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return;
	}

	if (APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
	{
		HandlePlayerControllerSet(PC);
	}

	LocalPlayer->OnPlayerControllerChanged().AddUObject(this, &ThisClass::HandlePlayerControllerSet);
}

void UWxUIManagerSubsystem::HandlePlayerControllerSet(APlayerController* PC)
{
	if (PC)
	{
		CreateLayoutForPlayer(PC);
	}
}

void UWxUIManagerSubsystem::CreateLayoutForPlayer(APlayerController* PC)
{
	if (!PC || PrimaryGameLayout)
	{
		return;
	}

	const UWxUIDeveloperSettings* UISettings = GetDefault<UWxUIDeveloperSettings>();
	if (!UISettings || UISettings->LayoutClass.IsNull())
	{
		return;
	}

	TSubclassOf<UWxPrimaryGameLayout> LayoutClass = UISettings->LayoutClass.LoadSynchronous();
	if (!LayoutClass)
	{
		return;
	}

	PrimaryGameLayout = CreateWidget<UWxPrimaryGameLayout>(PC, LayoutClass);
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->AddToPlayerScreen();
	}
}
