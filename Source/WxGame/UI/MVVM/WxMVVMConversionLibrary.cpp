// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxMVVMConversionLibrary.h"

#include "AttributeSet.h"
#include "UI/MVVM/WxViewModel_AbilitySystem.h"

ESlateVisibility UWxMVVMConversionLibrary::Conv_PositiveFloatToSlateVisibility(float Value, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return Value > 0.f ? TrueVisibility : FalseVisibility;
}

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}

ESlateVisibility UWxMVVMConversionLibrary::Conv_ObjectToSlateVisibility(const UObject* Object, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return IsValid(Object) ? TrueVisibility : FalseVisibility;
}

UWxViewModel_Attribute* UWxMVVMConversionLibrary::GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute)
{
	if (!AbilitySystem)
	{
		return nullptr;
	}
	return AbilitySystem->GetOrCreateAttributeViewModel(Attribute, MaxAttribute);
}

UWxViewModel_Ability* UWxMVVMConversionLibrary::GetAbilityViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTagContainer AbilityTags)
{
	return AbilitySystem ? AbilitySystem->GetOrCreateAbilityViewModel(AbilityTags) : nullptr;
}
