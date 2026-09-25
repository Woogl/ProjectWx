// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Character/WxCharacterBase.h"
#include "MVVM/WxViewModelResolver_AbilitySystem.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Character.h"

UObject* UWxViewModelResolver_PlayerCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const AWxCharacterBase* PlayerCharacter = PC ? PC->GetPawn<AWxCharacterBase>() : nullptr;
	if (!PlayerCharacter)
	{
		return nullptr;
	}
	return UWxViewModel_Character::GetOrCreate(
		UWxViewModelResolver_AbilitySystem::GetOrCreate(PlayerCharacter->GetAbilitySystemComponent()),
		PlayerCharacter->GetTitle(), PlayerCharacter->GetIcon());
}
