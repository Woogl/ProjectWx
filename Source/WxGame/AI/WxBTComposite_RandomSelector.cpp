// Copyright Woogle. All Rights Reserved.

#include "AI/WxBTComposite_RandomSelector.h"

#include "BehaviorTree/BTCompositeNode.h"

UWxBTComposite_RandomSelector::UWxBTComposite_RandomSelector(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Random Selector");
}

uint16 UWxBTComposite_RandomSelector::GetInstanceMemorySize() const
{
	return sizeof(FWxBTRandomSelectorMemory);
}

void UWxBTComposite_RandomSelector::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	if (InitType == EBTMemoryInit::Initialize)
	{
		FWxBTRandomSelectorMemory* Memory = reinterpret_cast<FWxBTRandomSelectorMemory*>(NodeMemory);
		Memory->TriedMask = 0;
		Memory->LastSucceededChild = INDEX_NONE;
	}
}

int32 UWxBTComposite_RandomSelector::GetNextChildHandler(FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	FWxBTRandomSelectorMemory* Memory = reinterpret_cast<FWxBTRandomSelectorMemory*>(GetNodeMemory<uint8>(SearchData));

	if (PrevChild == BTSpecialChild::NotInitialized)
	{
		Memory->TriedMask = 0;
	}
	else if (LastResult == EBTNodeResult::Succeeded)
	{
		if (PrevChild >= 0 && PrevChild < 32)
		{
			Memory->LastSucceededChild = PrevChild;
		}
		return BTSpecialChild::ReturnToParent;
	}
	else if (PrevChild >= 0 && PrevChild < 32)
	{
		Memory->TriedMask |= (1u << PrevChild);
	}

	const int32 ChildrenNum = FMath::Min(GetChildrenNum(), 32);
	const bool bShouldAvoid = bAvoidRepeat && Memory->TriedMask == 0 && Memory->LastSucceededChild != INDEX_NONE;

	TArray<int32, TInlineAllocator<32>> Untried;
	for (int32 Index = 0; Index < ChildrenNum; ++Index)
	{
		if ((Memory->TriedMask & (1u << Index)) != 0)
		{
			continue;
		}
		if (bShouldAvoid && Index == Memory->LastSucceededChild)
		{
			continue;
		}
		Untried.Add(Index);
	}

	// 폴백: 회피 때문에 후보가 비었으면 (자식 1개뿐인 경우 등) 직전 성공 자식 허용.
	if (Untried.Num() == 0 && bShouldAvoid &&
		Memory->LastSucceededChild >= 0 && Memory->LastSucceededChild < ChildrenNum)
	{
		Untried.Add(Memory->LastSucceededChild);
	}

	if (Untried.Num() == 0)
	{
		return BTSpecialChild::ReturnToParent;
	}

	return Untried[FMath::RandRange(0, Untried.Num() - 1)];
}

FString UWxBTComposite_RandomSelector::GetStaticDescription() const
{
	const FString Suffix = bAvoidRepeat ? TEXT(" [Avoid Repeat]") : TEXT("");
	return FString::Printf(TEXT("Random Selector: 자식들을 랜덤 순서로 시도, 하나 성공하면 성공%s"), *Suffix);
}
