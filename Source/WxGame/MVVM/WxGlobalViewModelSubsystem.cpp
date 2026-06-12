// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_Inventory.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"

void UWxGlobalViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PlayerCharacterViewModel = NewObject<UWxViewModel_Character>(this);
	BossCharacterViewModel = NewObject<UWxViewModel_Character>(this);
	InventoryViewModel = NewObject<UWxViewModel_Inventory>(this);
}

UWxViewModel_Character* UWxGlobalViewModelSubsystem::GetPlayerCharacterViewModel() const
{
	return PlayerCharacterViewModel;
}

UWxViewModel_Character* UWxGlobalViewModelSubsystem::GetBossCharacterViewModel() const
{
	return BossCharacterViewModel;
}

UWxViewModel_Inventory* UWxGlobalViewModelSubsystem::GetInventoryViewModel() const
{
	return InventoryViewModel;
}

UObject* UWxViewModelResolver_PlayerCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const ULocalPlayer* LocalPlayer = UserWidget ? UserWidget->GetOwningLocalPlayer() : nullptr;
	UWxGlobalViewModelSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>() : nullptr;
	return Subsystem ? Subsystem->GetPlayerCharacterViewModel() : nullptr;
}

UObject* UWxViewModelResolver_BossCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const ULocalPlayer* LocalPlayer = UserWidget ? UserWidget->GetOwningLocalPlayer() : nullptr;
	UWxGlobalViewModelSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>() : nullptr;
	return Subsystem ? Subsystem->GetBossCharacterViewModel() : nullptr;
}

