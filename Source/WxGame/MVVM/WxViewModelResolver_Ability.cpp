// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const IAbilitySystemInterface* AbilitySystemPawn = PC ? Cast<IAbilitySystemInterface>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = AbilitySystemPawn ? AbilitySystemPawn->GetAbilitySystemComponent() : nullptr;

	// 빈 컨테이너는 HasAll 이 항상 true 라 아무 어빌리티나 매칭된다.
	if (!ASC || AbilityTags.IsEmpty())
	{
		return nullptr;
	}

	// 슬롯 뷰모델의 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 슬롯을 보는 위젯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);

	return AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAbilityViewModel(AbilityTags) : nullptr;
}
