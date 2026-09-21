// Copyright Woogle. All Rights Reserved.

#include "WxInteractable.h"

bool IWxInteractable::CanInteract(const AActor* Interactor) const
{
	return true;
}

void IWxInteractable::GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const
{
	OutOptions.Add({GetInteractionPrompt(), INDEX_NONE});
}
