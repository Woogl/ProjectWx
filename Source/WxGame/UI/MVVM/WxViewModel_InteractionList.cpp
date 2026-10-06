// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_InteractionList.h"
#include "UI/MVVM/WxViewModel_Interaction.h"

void UWxViewModel_InteractionList::SetRows(const TArray<FText>& Prompts, int32 SelectedIndex)
{
	Entries.Reset(Prompts.Num());
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		UWxViewModel_Interaction* Entry = NewObject<UWxViewModel_Interaction>(this);
		Entry->Prompt = Prompts[Index];
		Entry->bSelected = Index == SelectedIndex;
		Entries.Add(Entry);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
}
