// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxMVVMConversionLibrary.h"

#include "AttributeSet.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}

UWxViewModel_Attribute* UWxMVVMConversionLibrary::GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute)
{
	if (!AbilitySystem)
	{
		return nullptr;
	}
	return AbilitySystem->GetOrCreateAttributeViewModel(Attribute, MaxAttribute);
}

