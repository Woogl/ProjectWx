// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "Interaction/WxInteractionComponent.h"

void UWxInteractionRegistrySubsystem::RegisterInRange(UWxInteractionComponent* Component)
{
	if (!Component || InRangeComponents.Contains(Component))
	{
		return;
	}

	InRangeComponents.Add(Component);
	RebuildAndNotify();
}

void UWxInteractionRegistrySubsystem::UnregisterInRange(UWxInteractionComponent* Component)
{
	if (InRangeComponents.RemoveSingle(Component) > 0)
	{
		// 목록에서 빠진 컴포넌트는 ApplyHighlight 대상이 아니므로 강조를 직접 끈다.
		if (Component)
		{
			Component->SetHighlightEnabled(false);
		}
		RebuildAndNotify();
	}
}

TArray<FText> UWxInteractionRegistrySubsystem::GetPrompts() const
{
	TArray<FText> Prompts;
	Prompts.Reserve(InRangeComponents.Num());
	for (const TWeakObjectPtr<UWxInteractionComponent>& Weak : InRangeComponents)
	{
		if (const UWxInteractionComponent* Component = Weak.Get())
		{
			Prompts.Add(Component->GetInteractionText());
		}
	}
	return Prompts;
}

UWxInteractionComponent* UWxInteractionRegistrySubsystem::GetSelectedComponent() const
{
	if (!InRangeComponents.IsValidIndex(SelectedIndex))
	{
		return nullptr;
	}
	return InRangeComponents[SelectedIndex].Get();
}

void UWxInteractionRegistrySubsystem::CycleSelection(int32 Delta)
{
	const int32 Count = InRangeComponents.Num();
	if (Count == 0 || Delta == 0)
	{
		return;
	}

	const int32 Base = (SelectedIndex == INDEX_NONE) ? 0 : SelectedIndex;
	const int32 NewIndex = ((Base + Delta) % Count + Count) % Count;
	UpdateSelection(NewIndex);
}

void UWxInteractionRegistrySubsystem::RebuildAndNotify()
{
	// 파괴된 참조를 정리한다.
	InRangeComponents.RemoveAll([](const TWeakObjectPtr<UWxInteractionComponent>& Weak) { return !Weak.IsValid(); });

	// 목록 변화에 맞춰 선택을 보정한다(비면 INDEX_NONE, 아니면 범위로 클램프; 첫 등록 시 0 선택).
	SelectedIndex = InRangeComponents.IsEmpty() ? INDEX_NONE : FMath::Clamp(SelectedIndex, 0, InRangeComponents.Num() - 1);

	ApplyHighlight();
	OnListChanged.Broadcast(GetPrompts());
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UWxInteractionRegistrySubsystem::UpdateSelection(int32 NewIndex)
{
	const int32 Clamped = InRangeComponents.IsEmpty() ? INDEX_NONE : FMath::Clamp(NewIndex, 0, InRangeComponents.Num() - 1);
	if (Clamped == SelectedIndex)
	{
		return;
	}

	SelectedIndex = Clamped;
	ApplyHighlight();
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UWxInteractionRegistrySubsystem::ApplyHighlight()
{
	for (int32 Index = 0; Index < InRangeComponents.Num(); ++Index)
	{
		if (UWxInteractionComponent* Component = InRangeComponents[Index].Get())
		{
			Component->SetHighlightEnabled(Index == SelectedIndex);
		}
	}
}
