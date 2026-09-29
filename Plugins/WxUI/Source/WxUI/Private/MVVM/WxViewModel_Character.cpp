// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Character.h"
#include "AbilitySystemComponent.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModelUtils.h"
#include "Types/MVVMViewModelCollection.h"

FMVVMViewModelContext UWxViewModel_Character::GetPlayerContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = StaticClass();
	Context.ContextName = TEXT("VM_PlayerCharacter");
	return Context;
}

UWxViewModel_Character* UWxViewModel_Character::FindPlayer(const UObject* WorldContextObject)
{
	UMVVMViewModelCollectionObject* Collection = WxViewModel::GetGlobalCollection(WorldContextObject);
	return Collection ? Cast<UWxViewModel_Character>(Collection->FindViewModelInstance(GetPlayerContext())) : nullptr;
}

void UWxViewModel_Character::Initialize(UAbilitySystemComponent* InASC, FText InCharacterName)
{
	Deinitialize();

	if (!InASC)
	{
		return;
	}

	UWxViewModel_AbilitySystem* NewAbilitySystem = NewObject<UWxViewModel_AbilitySystem>(this);
	NewAbilitySystem->Initialize(InASC);
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, NewAbilitySystem);
	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, InCharacterName);
}

void UWxViewModel_Character::Deinitialize()
{
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, nullptr);
	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, FText::GetEmpty());
}
