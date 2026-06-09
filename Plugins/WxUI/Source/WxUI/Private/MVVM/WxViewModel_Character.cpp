// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

void UWxViewModel_Character::Initialize(UAbilitySystemComponent* InASC, const FWxCharacterUIData& InUIData)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(this);
	AbilitySystemViewModel->Initialize(InASC);
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, AbilitySystemViewModel);

	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, InUIData.CharacterName);
	UE_MVVM_SET_PROPERTY_VALUE(Portrait, InUIData.Portrait);

	SetInitialized(true);
}

void UWxViewModel_Character::Deinitialize()
{
	if (AbilitySystem)
	{
		AbilitySystem->Deinitialize();
		AbilitySystem = nullptr;
	}

	CharacterName = FText::GetEmpty();
	Portrait = nullptr;

	SetInitialized(false);

	Super::Deinitialize();
}
