// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Inventory.h"
#include "MVVMGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

FMVVMViewModelContext UWxGlobalViewModelSubsystem::GetPlayerAbilitySystemContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = UWxViewModel_AbilitySystem::StaticClass();
	Context.ContextName = FName(TEXT("VM_PlayerAbilitySystem"));
	return Context;
}

FMVVMViewModelContext UWxGlobalViewModelSubsystem::GetBossAbilitySystemContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = UWxViewModel_AbilitySystem::StaticClass();
	Context.ContextName = FName(TEXT("VM_BossAbilitySystem"));
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

	PlayerAbilitySystemViewModel = RegisterGlobalViewModel<UWxViewModel_AbilitySystem>(GetPlayerAbilitySystemContext());
	BossAbilitySystemViewModel = RegisterGlobalViewModel<UWxViewModel_AbilitySystem>(GetBossAbilitySystemContext());
	InventoryViewModel = RegisterGlobalViewModel<UWxViewModel_Inventory>(GetInventoryContext());
}

void UWxGlobalViewModelSubsystem::Deinitialize()
{
	UnregisterGlobalViewModel(GetPlayerAbilitySystemContext(), PlayerAbilitySystemViewModel);
	UnregisterGlobalViewModel(GetBossAbilitySystemContext(), BossAbilitySystemViewModel);
	UnregisterGlobalViewModel(GetInventoryContext(), InventoryViewModel);

	Super::Deinitialize();
}

UWxViewModel_AbilitySystem* UWxGlobalViewModelSubsystem::GetPlayerAbilitySystemViewModel() const
{
	return PlayerAbilitySystemViewModel;
}

UWxViewModel_AbilitySystem* UWxGlobalViewModelSubsystem::GetBossAbilitySystemViewModel() const
{
	return BossAbilitySystemViewModel;
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

