// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeCondition_BeyondLeash.h"

#include "AI/WxAIController.h"
#include "Minion/WxMinionComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

bool FWxStateTreeCondition_BeyondLeash::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AWxAIController* AIController = Cast<AWxAIController>(Context.GetOwner());
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	FVector AnchorLocation = AIController->GetHomeLocation();
	if (Anchor == EWxLeashAnchor::Master)
	{
		const APawn* Master = UWxMinionComponent::GetMaster(*Pawn);
		if (!Master)
		{
			return false;
		}
		AnchorLocation = Master->GetActorLocation();
	}

	const double DistSquared = FVector::DistSquared(Pawn->GetActorLocation(), AnchorLocation);
	return DistSquared > FMath::Square(Instance.LeashRadius);
}

#if WITH_EDITOR
FText FWxStateTreeCondition_BeyondLeash::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	FText RadiusValue = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, LeashRadius)), Formatting);
	if (RadiusValue.IsEmpty())
	{
		RadiusValue = FText::AsNumber(InstanceData->LeashRadius);
	}

	return FText::Format(INVTEXT("{0}에서 {1} 넘게 이탈"), Anchor == EWxLeashAnchor::Master ? INVTEXT("주인") : INVTEXT("홈"), RadiusValue);
}
#endif
