// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "Interaction/WxInteractionComponent.h"

void UWxInteractionRegistrySubsystem::UpdateInRange(const TArray<UWxInteractionComponent*>& InCandidates)
{
	// 선택 안정성을 위해 갱신 전 선택 컴포넌트를 포인터로 캐시한다. 순서가 바뀌어도 동일 컴포넌트를 다시 찾아 선택을 잇는다.
	UWxInteractionComponent* PreviousSelected = GetSelectedComponent();

	bool bChanged = false;

	// 이탈/파괴 제거: 새 후보 집합에 없는 기존 항목을 떼고 강조를 끈다.
	for (int32 Index = InRangeComponents.Num() - 1; Index >= 0; --Index)
	{
		UWxInteractionComponent* Existing = InRangeComponents[Index].Get();
		if (!Existing || !InCandidates.Contains(Existing))
		{
			if (Existing)
			{
				Existing->SetHighlightEnabled(false);
			}
			InRangeComponents.RemoveAt(Index);
			bChanged = true;
		}
	}

	// 신규 추가: 기존에 없던 후보를 뒤에 붙인다(후보는 거리순이라 가까운 것부터 들어온다).
	for (UWxInteractionComponent* Candidate : InCandidates)
	{
		if (Candidate && !InRangeComponents.Contains(Candidate))
		{
			InRangeComponents.Add(Candidate);
			bChanged = true;
		}
	}

	if (!bChanged)
	{
		return;
	}

	// 선택 복원: 캐시한 컴포넌트가 남아 있으면 그 인덱스로, 없으면 비었을 때 INDEX_NONE / 아니면 0.
	const int32 RestoredIndex = PreviousSelected ? InRangeComponents.IndexOfByKey(PreviousSelected) : INDEX_NONE;
	SelectedIndex = InRangeComponents.IsEmpty() ? INDEX_NONE : (RestoredIndex != INDEX_NONE ? RestoredIndex : 0);

	ApplyHighlight();
	OnListChanged.Broadcast(GetPrompts());
	OnSelectionChanged.Broadcast(SelectedIndex);
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
