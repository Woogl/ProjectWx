// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxMVVMConversionLibrary.h"
#include "MVVM/WxViewModel_Ability.h"

bool UWxMVVMConversionLibrary::Conv_GameplayTagToBool(const FGameplayTagContainer& TagContainer, FGameplayTag Tag)
{
	return TagContainer.HasTag(Tag);
}

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}

UWxViewModel_Ability* UWxMVVMConversionLibrary::Conv_FindAbilityViewModelByTag(const TArray<UWxViewModel_Ability*>& AbilityViewModels, FGameplayTag AbilityTag)
{
	for (UWxViewModel_Ability* ViewModel : AbilityViewModels)
	{
		if (ViewModel && ViewModel->GetAbilityTag() == AbilityTag)
		{
			return ViewModel;
		}
	}
	return nullptr;
}
