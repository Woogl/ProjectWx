// Copyright Woogle. All Rights Reserved.

#include "Input/WxInputConfig.h"

const UInputAction* UWxInputConfig::FindInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FWxInputAbilityBinding& Binding : AbilityInputBindings)
	{
		if (Binding.InputTag == InputTag)
		{
			return Binding.InputAction;
		}
	}
	return nullptr;
}

FGameplayTag UWxInputConfig::FindTagForInputAction(const UInputAction* InputAction) const
{
	for (const FWxInputAbilityBinding& Binding : AbilityInputBindings)
	{
		if (Binding.InputAction == InputAction)
		{
			return Binding.InputTag;
		}
	}
	return FGameplayTag();
}
