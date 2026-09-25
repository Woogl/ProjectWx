// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemInterface.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModelResolver_AbilitySystem.h"

namespace
{
	void ApplyAbilityPresentation(UWxViewModel_Ability& ViewModel, const UGameplayAbility* Ability)
	{
		if (const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(Ability))
		{
			ViewModel.SetPresentation(WxAbility->GetTitle(), WxAbility->GetDescription(), WxAbility->GetIcon(),
				WxAbility->GetMaxRecharges(), WxAbility->GetCooldownTime());
		}
	}
}

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const IAbilitySystemInterface* AbilitySystemPawn = PC ? Cast<IAbilitySystemInterface>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = AbilitySystemPawn ? AbilitySystemPawn->GetAbilitySystemComponent() : nullptr;
	return GetOrCreate(ASC, AbilityTags);
}

UWxViewModel_Ability* UWxViewModelResolver_Ability::GetOrCreate(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InAbilityTags)
{
	// 빈 컨테이너는 HasAll 이 항상 true 라 아무 어빌리티나 매칭된다.
	if (!InASC || InAbilityTags.IsEmpty())
	{
		return nullptr;
	}

	// 슬롯 뷰모델의 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 슬롯을 보는 위젯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModelResolver_AbilitySystem::GetOrCreate(InASC);
	if (!AbilitySystemViewModel)
	{
		return nullptr;
	}

	return AbilitySystemViewModel->GetOrCreateAbilityViewModel(InAbilityTags, FWxOnBoundAbilityChanged::CreateStatic(&ApplyAbilityPresentation));
}
