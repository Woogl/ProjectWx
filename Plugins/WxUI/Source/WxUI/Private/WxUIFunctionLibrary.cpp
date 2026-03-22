// Copyright Woogle. All Rights Reserved.

#include "WxUIFunctionLibrary.h"

bool UWxUIFunctionLibrary::HasGameplayTag(const FGameplayTagContainer& TagContainer, FGameplayTag Tag)
{
	return TagContainer.HasTag(Tag);
}
