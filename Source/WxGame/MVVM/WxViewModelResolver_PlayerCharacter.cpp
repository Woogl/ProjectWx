// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
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

	// 위젯이 아닌 데이터 소스(빙의 캐릭터)를 Outer 로 생성한다.
	// 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_Character* ViewModel = NewObject<UWxViewModel_Character>(PlayerCharacter);
	ViewModel->Initialize(ASC, PlayerCharacter->GetCharacterName(), PlayerCharacter->GetPortrait());

	return ViewModel;
}
