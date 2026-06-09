// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_Inventory.h"
#include "MVVMGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

FMVVMViewModelContext UWxGlobalViewModelSubsystem::GetPlayerCharacterContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = UWxViewModel_Character::StaticClass();
	Context.ContextName = FName(TEXT("VM_PlayerCharacter"));
	return Context;
}

FMVVMViewModelContext UWxGlobalViewModelSubsystem::GetBossCharacterContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = UWxViewModel_Character::StaticClass();
	Context.ContextName = FName(TEXT("VM_BossCharacter"));
	return Context;
}

FMVVMViewModelContext UWxGlobalViewModelSubsystem::GetInventoryContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = UWxViewModel_Inventory::StaticClass();
	Context.ContextName = FName(TEXT("VM_Inventory"));
	return Context;
}

void UWxGlobalViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PlayerCharacterViewModel = RegisterGlobalViewModel<UWxViewModel_Character>(GetPlayerCharacterContext());
	BossCharacterViewModel = RegisterGlobalViewModel<UWxViewModel_Character>(GetBossCharacterContext());
	InventoryViewModel = RegisterGlobalViewModel<UWxViewModel_Inventory>(GetInventoryContext());
}

void UWxGlobalViewModelSubsystem::Deinitialize()
{
	UnregisterGlobalViewModel(GetPlayerCharacterContext(), PlayerCharacterViewModel);
	UnregisterGlobalViewModel(GetBossCharacterContext(), BossCharacterViewModel);
	UnregisterGlobalViewModel(GetInventoryContext(), InventoryViewModel);

	Super::Deinitialize();
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

UMVVMViewModelCollectionObject* UWxGlobalViewModelSubsystem::GetGlobalCollection() const
{
	UGameInstance* GameInst = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	if (!GameInst)
	{
		return nullptr;
	}

	UMVVMGameSubsystem* MVVMGameSubsystem = GameInst->GetSubsystem<UMVVMGameSubsystem>();
	if (!MVVMGameSubsystem)
	{
		return nullptr;
	}

	return MVVMGameSubsystem->GetViewModelCollection();
}

