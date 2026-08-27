// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const AWxPlayerCharacter* PlayerCharacter = PC ? Cast<AWxPlayerCharacter>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = PlayerCharacter ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;

	// 어빌리티 매칭과 인스턴스 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 어빌리티를 보는 슬롯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);
	UWxViewModel_Ability* ViewModel = AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAbilityViewModel(AbilityTags) : nullptr;
	if (!ViewModel)
	{
		return nullptr;
	}

	if (const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ViewModel->GetBoundAbility()))
	{
		// 공유본이라 앞서 채워졌으면 다시 스트리밍하지 않는다.
		if (!ViewModel->GetIcon())
		{
			ViewModel->SetIconSoft(Ability->GetIcon());
		}

		ViewModel->SetMaxRecharges(Ability->GetMaxRecharges());
	}

	return ViewModel;
}
