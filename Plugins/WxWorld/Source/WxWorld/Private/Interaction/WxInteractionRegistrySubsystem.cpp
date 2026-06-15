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

void UWxInteractionRegistrySubsystem::RebuildAndNotify()
{
	// 파괴된 참조를 정리한다.
	InRangeComponents.RemoveAll([](const TWeakObjectPtr<UWxInteractionComponent>& Weak) { return !Weak.IsValid(); });

	OnListChanged.Broadcast(GetPrompts());
}
