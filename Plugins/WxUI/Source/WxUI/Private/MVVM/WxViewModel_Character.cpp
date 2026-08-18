// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

#include "Engine/Texture2D.h"

void UWxViewModel_Character::Initialize(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = NewObject<UWxViewModel_AbilitySystem>(this);
	AbilitySystemViewModel->Initialize(InASC);
	UE_MVVM_SET_PROPERTY_VALUE(AbilitySystem, AbilitySystemViewModel);

	UE_MVVM_SET_PROPERTY_VALUE(CharacterName, InCharacterName);
	RequestImageAsync(TEXT("Portrait"), InPortrait);
}

void UWxViewModel_Character::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	UE_MVVM_SET_PROPERTY_VALUE(Portrait, LoadedImage);
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

	Super::Deinitialize();
}
