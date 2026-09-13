// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Character.h"

UObject* UWxViewModelResolver_PlayerCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	APawn* PlayerCharacter = PC ? PC->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = PlayerCharacter ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter) : nullptr;
	return UWxViewModel_Character::GetOrCreate(ASC, PlayerCharacter);
}
