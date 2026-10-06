// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeCondition_TargetDistance.h"

#include "AI/WxAIController.h"
#include "Conditions/StateTreeConditionHelpers.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include "StateTreePropertyBindings.h"

bool FWxStateTreeCondition_TargetDistance::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AWxAIController* AIController = Cast<AWxAIController>(Context.GetOwner());
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const AActor* Target = AIController ? AIController->GetTargetActor() : nullptr;

	double TargetDistance = TNumericLimits<double>::Max();
	if (Pawn && Target)
	{
		TargetDistance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
	}

	return UE::StateTree::Conditions::CompareNumbers<double>(TargetDistance, Instance.Distance, Operator);
}

#if WITH_EDITOR
FText FWxStateTreeCondition_TargetDistance::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	FText DistanceValue = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Distance)), Formatting);
	if (DistanceValue.IsEmpty())
	{
		DistanceValue = FText::AsNumber(InstanceData->Distance);
	}

	return FText::Format(INVTEXT("Target Distance {0} {1}"), UE::StateTree::DescHelpers::GetOperatorText(Operator, Formatting), DistanceValue);
}
#endif
