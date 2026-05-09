// Copyright Woogle. All Rights Reserved.

#include "WxBTComposite_RandomChoice.h"

#include "BehaviorTree/BTCompositeNode.h"

UWxBTComposite_RandomChoice::UWxBTComposite_RandomChoice(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Random Choice");
}

uint16 UWxBTComposite_RandomChoice::GetInstanceMemorySize() const
{
	return sizeof(int32);
}

void UWxBTComposite_RandomChoice::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	if (InitType == EBTMemoryInit::Initialize)
	{
		// 직전 진입에서 선택된 자식 인덱스. 회피 비교 기준. INDEX_NONE = 미설정.
		*reinterpret_cast<int32*>(NodeMemory) = INDEX_NONE;
	}
}

int32 UWxBTComposite_RandomChoice::GetNextChildHandler(FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	// 첫 진입이 아니면 (= 선택된 자식이 결과를 반환한 시점) 결과를 그대로 부모에 전파한다. 폴백 없음.
	if (PrevChild != BTSpecialChild::NotInitialized)
	{
		return BTSpecialChild::ReturnToParent;
	}

	const int32 ChildrenNum = GetChildrenNum();
	if (ChildrenNum <= 0)
	{
		return BTSpecialChild::ReturnToParent;
	}

	int32& LastChosenChild = *reinterpret_cast<int32*>(GetNodeMemory<uint8>(SearchData));

	const bool bShouldAvoid = bAvoidRepeat && LastChosenChild != INDEX_NONE && ChildrenNum > 1;

	TArray<int32, TInlineAllocator<8>> Candidates;
	Candidates.Reserve(ChildrenNum);
	for (int32 Index = 0; Index < ChildrenNum; ++Index)
	{
		if (bShouldAvoid && Index == LastChosenChild)
		{
			continue;
		}
		Candidates.Add(Index);
	}

	if (Candidates.Num() == 0)
	{
		return BTSpecialChild::ReturnToParent;
	}

	const int32 Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	LastChosenChild = Chosen;
	return Chosen;
}

FString UWxBTComposite_RandomChoice::GetStaticDescription() const
{
	return FString::Printf(TEXT("자식 중 무작위 1개를 골라 실행"));
}
