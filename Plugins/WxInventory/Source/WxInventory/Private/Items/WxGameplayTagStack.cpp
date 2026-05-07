// Copyright Woogle. All Rights Reserved.

#include "Items/WxGameplayTagStack.h"

FWxGameplayTagStack::FWxGameplayTagStack()
	: StackCount(0)
{
}

FWxGameplayTagStack::FWxGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
	: Tag(InTag)
	, StackCount(InStackCount)
{
}

FString FWxGameplayTagStack::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%d"), *Tag.ToString(), StackCount);
}

FWxGameplayTagStackContainer::FWxGameplayTagStackContainer()
{
}

void FWxGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid() || StackCount <= 0)
	{
		return;
	}

	for (FWxGameplayTagStack& Stack : Stacks)
	{
		if (Stack.Tag == Tag)
		{
			Stack.StackCount += StackCount;
			TagToCountMap[Tag] = Stack.StackCount;
			MarkItemDirty(Stack);
			return;
		}
	}

	FWxGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
	MarkItemDirty(NewStack);
	TagToCountMap.Add(Tag, StackCount);
}

void FWxGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid() || StackCount <= 0)
	{
		return;
	}

	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FWxGameplayTagStack& Stack = *It;
		if (Stack.Tag != Tag)
		{
			continue;
		}

		if (Stack.StackCount <= StackCount)
		{
			TagToCountMap.Remove(Tag);
			It.RemoveCurrent();
			MarkArrayDirty();
		}
		else
		{
			Stack.StackCount -= StackCount;
			TagToCountMap[Tag] = Stack.StackCount;
			MarkItemDirty(Stack);
		}
		return;
	}
}

int32 FWxGameplayTagStackContainer::GetStackCount(FGameplayTag Tag) const
{
	return TagToCountMap.FindRef(Tag);
}

bool FWxGameplayTagStackContainer::ContainsTag(FGameplayTag Tag) const
{
	return TagToCountMap.Contains(Tag);
}

void FWxGameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToCountMap.Remove(Tag);
	}
}

void FWxGameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FWxGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
	}
}

void FWxGameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FWxGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap[Stack.Tag] = Stack.StackCount;
	}
}

bool FWxGameplayTagStackContainer::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FWxGameplayTagStack, FWxGameplayTagStackContainer>(Stacks, DeltaParms, *this);
}
