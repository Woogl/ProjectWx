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

	TArray<int32, TInlineAllocator<8>> Candidates;
	TArray<float, TInlineAllocator<8>> Weights;
	Candidates.Reserve(ChildrenNum);
	Weights.Reserve(ChildrenNum);

	// 회피는 여기서 보지 않는다. 회피를 풀지 말지는 조건을 통과한 후보가 몇 개인지에 달렸으므로, 수집을 끝낸 뒤에 판단해야 한다.
	for (int32 Index = 0; Index < ChildrenNum; ++Index)
	{
		// 엔진이 선택 직후 FindChildToExecute 에서 이 자식에 대해 동일하게 호출하는 검사이므로, 미리 걸러도 선택 결과가 엔진 판정과 어긋나지 않는다.
		if (!DoDecoratorsAllowExecution(SearchData.OwnerComp, SearchData.OwnerComp.GetActiveInstanceIdx(), Index))
		{
			// 엔진은 FindChildToExecute 에서 조건 실패 자식을 지나칠 때 이 알림으로 LowerPriority·Both 데코레이터를 관찰자로 등록한다.
			// 사전 필터가 그 경로를 건너뛰므로 여기서 대신 보낸다.
			EBTNodeResult::Type FailedResult = EBTNodeResult::Failed;
			NotifyDecoratorsOnFailedActivation(SearchData, Index, FailedResult);
			continue;
		}

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

		// 회피를 풀지 말지가 후보 수 기준이므로, 뽑힐 수 없는 자식이 그 수에 끼면 안 된다.
		// 조건은 통과한 자식이라 활성화 실패로 알리지 않는다 — 뽑힐 수 없는데 관찰자로 등록하면 선점만 하고 정작 실행되지 못한다.
		if (Weight <= 0.0f)
		{
			continue;
		}

		Candidates.Add(Index);
		Weights.Add(Weight);
	}

	if (Candidates.Num() == 0)
	{
		return BTSpecialChild::ReturnToParent;
	}

	if (bAvoidRepeat && Candidates.Num() > 1)
	{
		const int32 RepeatIndex = Candidates.Find(LastChosenChild);
		if (RepeatIndex != INDEX_NONE)
		{
			Candidates.RemoveAt(RepeatIndex);
			Weights.RemoveAt(RepeatIndex);
		}
	}

	float TotalWeight = 0.0f;
	for (const float Weight : Weights)
	{
		TotalWeight += Weight;
	}

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
