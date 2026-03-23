// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxMVVMConversionLibrary.h"

bool UWxMVVMConversionLibrary::Conv_GameplayTagToBool(const FGameplayTagContainer& TagContainer, FGameplayTag Tag)
{
	return TagContainer.HasTag(Tag);
}

ESlateVisibility UWxMVVMConversionLibrary::Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility, ESlateVisibility FalseVisibility)
{
	return TagContainer.HasTag(Tag) ? TrueVisibility : FalseVisibility;
}
