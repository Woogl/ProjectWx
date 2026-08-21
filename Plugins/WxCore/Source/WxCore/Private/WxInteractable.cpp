// Copyright Woogle. All Rights Reserved.

#include "WxInteractable.h"

void IWxInteractable::SetInteractionEnabled(bool bEnabled)
{
}

bool IWxInteractable::CanBeInteractedBy(const AActor* Interactor) const
{
	return true;
}
