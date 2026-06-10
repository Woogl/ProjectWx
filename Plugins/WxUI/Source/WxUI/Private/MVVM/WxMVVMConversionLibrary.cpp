// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxMVVMConversionLibrary.h"

#include "AttributeSet.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}

UWxViewModel_Attribute* UWxMVVMConversionLibrary::GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute CurrentAttribute, FGameplayAttribute MaxAttribute)
{
	if (!AbilitySystem)
	{
		return nullptr;
	}
	return AbilitySystem->GetOrCreateAttributeViewModel(CurrentAttribute, MaxAttribute);
}

UWxViewModel_Ability* UWxMVVMConversionLibrary::GetAbilityViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTagContainer AbilityTags)
{
	if (!AbilitySystem)
	{
		return nullptr;
	}
	return AbilitySystem->GetOrCreateAbilityViewModel(AbilityTags);
}

UWxViewModel_Effect* UWxMVVMConversionLibrary::FindActiveEffectViewModelByTag(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTag EffectTag)
{
	if (!AbilitySystem)
	{
		return nullptr;
	}
	return AbilitySystem->FindActiveEffectViewModel(EffectTag);
}
