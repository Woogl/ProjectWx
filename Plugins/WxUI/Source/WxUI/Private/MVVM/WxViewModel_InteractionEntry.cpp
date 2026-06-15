// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_InteractionEntry.h"

void UWxViewModel_InteractionEntry::SetPrompt(const FText& InPrompt)
{
	if (Prompt.IdenticalTo(InPrompt))
	{
		return;
	}

	Prompt = InPrompt;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Prompt);
}

void UWxViewModel_InteractionEntry::SetSelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}

	bSelected = bInSelected;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bSelected);
}
