// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel.h"
#include "MVVM/WxViewModel_Character.h"

UObject* UWxViewModelResolver_PlayerCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	AWxPlayerCharacter* PlayerCharacter = PC ? Cast<AWxPlayerCharacter>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = PlayerCharacter ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	// 위젯이 아닌 데이터 소스(ASC)를 Outer 로 생성해, 같은 캐릭터를 보는 다른 위젯이 여기서 찾아 쓰게 한다.
	if (UWxViewModel* Existing = UWxViewModel::FindSharedViewModel(ASC, UWxViewModel_Character::StaticClass()))
	{
		return Existing;
	}

	UWxViewModel_Character* ViewModel = NewObject<UWxViewModel_Character>(ASC);
	ViewModel->Initialize(ASC, PlayerCharacter->GetCharacterName(), PlayerCharacter->GetPortrait());

	return ViewModel;
}
