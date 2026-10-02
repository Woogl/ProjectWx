// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Character.h"
#include "AbilitySystemComponent.h"
#include "UI/MVVM/WxViewModel_AbilitySystem.h"
#include "UI/MVVM/WxViewModelUtils.h"
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
	// 새로 만들면 위젯 바인딩과 자식 VM 이 전부 다시 만들어지므로, 같은 ASC 면 유지한다.
	if (InASC && AbilitySystem && AbilitySystem->GetBoundASC() == InASC)
	{
		UE_MVVM_SET_PROPERTY_VALUE(CharacterName, InCharacterName);
		return;
	}

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
	UWxViewModel_AbilitySystem* PreviousAbilitySystem = AbilitySystem;
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, nullptr);
	if (PreviousAbilitySystem)
	{
		PreviousAbilitySystem->Deinitialize();
	}
	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, FText::GetEmpty());
}
