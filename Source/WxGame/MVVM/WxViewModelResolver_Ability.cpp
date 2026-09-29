// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModelResolver_AbilitySystem.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;

	// 슬롯 뷰모델의 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 슬롯을 보는 위젯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModelResolver_AbilitySystem::GetOrCreate(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC ? PC->GetPawn() : nullptr));
	if (!AbilitySystemViewModel)
	{
		return nullptr;
	}

	return AbilitySystemViewModel->GetOrCreateAbilityViewModel(AbilityTags,
		FWxOnBoundAbilityChanged::CreateLambda([](UWxViewModel_Ability& ViewModel, const UGameplayAbility* Ability)
		{
			if (const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(Ability))
			{
				ViewModel.SetPresentation(WxAbility->GetTitle(), WxAbility->GetDescription(), WxAbility->GetIcon(),
					WxAbility->GetMaxRecharges(), WxAbility->GetCooldownTime());
			}
		}),
		FWxCanBindAbility::CreateLambda([](const UAbilitySystemComponent& ASC, const UGameplayAbility& Ability)
		{
			const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(&Ability);
			return WxAbility ? WxAbility->DoesOwnerSatisfyActivationTags(ASC) : Ability.DoesAbilitySatisfyTagRequirements(ASC);
		}));
}
