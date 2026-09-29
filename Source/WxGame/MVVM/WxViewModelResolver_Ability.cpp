// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "Blueprint/UserWidget.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Character.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	// 슬롯 뷰모델의 소유는 플레이어 Character VM 의 어빌리티시스템 VM 이 맡는다 — 같은 슬롯을 보는 위젯끼리 하나를 나눠 쓴다.
	const UWxViewModel_Character* PlayerViewModel = UWxViewModel_Character::FindPlayer(UserWidget);
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = PlayerViewModel ? PlayerViewModel->AbilitySystem.Get() : nullptr;
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
