// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Interaction.h"
#include "MVVM/WxViewModel_InteractionEntry.h"

void UWxViewModel_Interaction::Initialize(const TArray<FText>& InPrompts, int32 InSelectedIndex)
{
	RebuildEntries(InPrompts);
	ApplySelection(InSelectedIndex);
	SetInitialized(true);
}

void UWxViewModel_Interaction::Deinitialize()
{
	if (!Entries.IsEmpty())
	{
		Entries.Reset();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
	}

	SetInitialized(false);
}

void UWxViewModel_Interaction::HandleListChanged(const TArray<FText>& InPrompts)
{
	RebuildEntries(InPrompts);

	// 목록이 새 엔트리로 교체되었으므로 현재 선택을 다시 적용해 bSelected 를 반영한다.
	ApplySelection(SelectedIndex);
}

void UWxViewModel_Interaction::SetSelectedIndex(int32 InSelectedIndex)
{
	ApplySelection(InSelectedIndex);
}

void UWxViewModel_Interaction::RebuildEntries(const TArray<FText>& InPrompts)
{
	Entries.Reset(InPrompts.Num());
	for (const FText& Prompt : InPrompts)
	{
		UWxViewModel_InteractionEntry* Entry = NewObject<UWxViewModel_InteractionEntry>(this);
		Entry->SetPrompt(Prompt);
		Entries.Add(Entry);
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
}

void UWxViewModel_Interaction::ApplySelection(int32 InSelectedIndex)
{
	const int32 ClampedIndex = Entries.IsEmpty() ? INDEX_NONE : FMath::Clamp(InSelectedIndex, 0, Entries.Num() - 1);

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (UWxViewModel_InteractionEntry* Entry = Entries[Index])
		{
			Entry->SetSelected(Index == ClampedIndex);
		}
	}

	if (SelectedIndex != ClampedIndex)
	{
		SelectedIndex = ClampedIndex;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedIndex);
	}
}
