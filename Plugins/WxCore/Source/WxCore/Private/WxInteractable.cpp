// Copyright Woogle. All Rights Reserved.

#include "WxInteractable.h"

void IWxInteractable::SetInteractionEnabled(bool bEnabled)
{
}

bool IWxInteractable::CanInteract(const AActor* Interactor) const
{
	return true;
}
