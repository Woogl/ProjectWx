// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
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

void UWxGlobalViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PlayerAbilitySystemViewModel = RegisterGlobalViewModel<UWxViewModel_AbilitySystem>(GetPlayerAbilitySystemContext());
	BossAbilitySystemViewModel = RegisterGlobalViewModel<UWxViewModel_AbilitySystem>(GetBossAbilitySystemContext());
}

void UWxGlobalViewModelSubsystem::Deinitialize()
{
	UnregisterGlobalViewModel(GetPlayerAbilitySystemContext(), PlayerAbilitySystemViewModel);
	UnregisterGlobalViewModel(GetBossAbilitySystemContext(), BossAbilitySystemViewModel);

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

