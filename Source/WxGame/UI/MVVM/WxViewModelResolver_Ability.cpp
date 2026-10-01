// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModelResolver_Ability.h"
#include "Blueprint/UserWidget.h"
#include "UI/MVVM/WxViewModel_Ability.h"
#include "UI/MVVM/WxViewModel_AbilitySystem.h"
#include "UI/MVVM/WxViewModel_Character.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	// 슬롯 뷰모델을 어빌리티시스템 VM 이 소유해 같은 슬롯을 보는 위젯끼리 하나를 나눠 쓴다.
	const UWxViewModel_Character* PlayerViewModel = UWxViewModel_Character::FindPlayer(UserWidget);
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = PlayerViewModel ? PlayerViewModel->AbilitySystem.Get() : nullptr;
	if (!AbilitySystemViewModel)
	{
		return nullptr;
	}

	return AbilitySystemViewModel->GetOrCreateAbilityViewModel(AbilityTags);
}
