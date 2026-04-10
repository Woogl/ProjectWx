// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxMVVMConversionLibrary.h"
#include "MVVM/WxViewModel_Attribute.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_Effect.h"

bool UWxMVVMConversionLibrary::Conv_GameplayTagToBool(const FGameplayTagContainer& TagContainer, FGameplayTag Tag)
{
	return TagContainer.HasTag(Tag);
}

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}

UWxViewModel_Attribute* UWxMVVMConversionLibrary::Conv_FindAttributeViewModel(const TArray<UWxViewModel_Attribute*>& AttributeViewModels, FGameplayAttribute Attribute)
{
	for (UWxViewModel_Attribute* ViewModel : AttributeViewModels)
	{
		if (ViewModel && ViewModel->GetBoundAttribute() == Attribute)
		{
			return ViewModel;
		}
	}
	return nullptr;
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

UWxViewModel_Effect* UWxMVVMConversionLibrary::Conv_FindActiveEffectViewModelByTag(const TArray<UWxViewModel_Effect*>& EffectViewModels, FGameplayTag EffectTag)
{
	for (UWxViewModel_Effect* ViewModel : EffectViewModels)
	{
		if (ViewModel && ViewModel->GetEffectTag() == EffectTag)
		{
			return ViewModel;
		}
	}
	return nullptr;
}
