// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
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

	// 어빌리티 매칭과 인스턴스 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 어빌리티를 보는 슬롯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);
	UWxViewModel_Ability* ViewModel = AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAbilityViewModel(AbilityTags) : nullptr;
	if (!ViewModel)
	{
		return nullptr;
	}

	if (const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ViewModel->GetBoundAbility()))
	{
		ViewModel->SetMaxRecharges(Ability->GetMaxRecharges());
	}

	return ViewModel;
}
