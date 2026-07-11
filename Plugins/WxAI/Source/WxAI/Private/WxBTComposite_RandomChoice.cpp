// Copyright Woogle. All Rights Reserved.

#include "WxBTComposite_RandomChoice.h"

#include "WxBTDecorator_RandomWeight.h"

UWxBTComposite_RandomChoice::UWxBTComposite_RandomChoice(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Random Choice");
}

uint16 UWxBTComposite_RandomChoice::GetInstanceMemorySize() const
{
	return sizeof(FWxBTRandomChoiceMemory);
}

void UWxBTComposite_RandomChoice::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	if (InitType == EBTMemoryInit::Initialize)
	{
		FWxBTRandomChoiceMemory* Memory = reinterpret_cast<FWxBTRandomChoiceMemory*>(NodeMemory);
		Memory->LastChosenChild = INDEX_NONE;
	}
}

FString UWxBTComposite_RandomChoice::GetStaticDescription() const
{
	return FString::Printf(TEXT("AvoidRepeat = %s"), bAvoidRepeat ? TEXT("true") : TEXT("false"));
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

	FWxBTRandomChoiceMemory* Memory = reinterpret_cast<FWxBTRandomChoiceMemory*>(GetNodeMemory<uint8>(SearchData));
	int32& LastChosenChild = Memory->LastChosenChild;

	const bool bShouldAvoid = bAvoidRepeat && LastChosenChild != INDEX_NONE && ChildrenNum > 1;

	TArray<int32, TInlineAllocator<8>> Candidates;
	TArray<float, TInlineAllocator<8>> Weights;
	Candidates.Reserve(ChildrenNum);
	Weights.Reserve(ChildrenNum);
	float TotalWeight = 0.0f;

	for (int32 Index = 0; Index < ChildrenNum; ++Index)
	{
		if (bShouldAvoid && Index == LastChosenChild)
		{
			continue;
		}

		// 자식에 붙은 Weight Decorator 중 첫 번째 것의 가중치를 사용한다. 없으면 기본 1.0.
		float Weight = 1.0f;
		for (const UBTDecorator* Decorator : Children[Index].Decorators)
		{
			const UWxBTDecorator_RandomWeight* WeightDecorator = Cast<UWxBTDecorator_RandomWeight>(Decorator);
			if (WeightDecorator)
			{
				Weight = WeightDecorator->GetWeight();
				break;
			}
		}

		Candidates.Add(Index);
		Weights.Add(Weight);
		TotalWeight += Weight;
	}

	// 후보가 없거나(전부 회피됨) 모든 후보 가중치가 0 이면 실행할 자식이 없으므로 부모에 실패를 반환한다.
	if (Candidates.Num() == 0 || TotalWeight <= 0.0f)
	{
		return BTSpecialChild::ReturnToParent;
	}

	// 누적 가중치 룰렛: [0, TotalWeight) 난수를 뽑아 누적합이 처음으로 이를 넘는 후보를 고른다.
	const float Roll = FMath::FRandRange(0.0f, TotalWeight);
	float Accumulated = 0.0f;
	int32 Chosen = Candidates.Last(); // 부동소수 경계로 루프가 못 고를 때의 폴백
	for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		Accumulated += Weights[CandidateIndex];
		if (Roll < Accumulated)
		{
			Chosen = Candidates[CandidateIndex];
			break;
		}
	}

	LastChosenChild = Chosen;
	return Chosen;
}
