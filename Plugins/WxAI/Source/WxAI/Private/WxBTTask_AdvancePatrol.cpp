// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_AdvancePatrol.h"

#include "WxPatrolRouteProvider.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_AdvancePatrol::UWxBTTask_AdvancePatrol()
{
	NodeName = TEXT("Advance Patrol");
}

EBTNodeResult::Type UWxBTTask_AdvancePatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	IWxPatrolRouteProvider* Provider = Cast<IWxPatrolRouteProvider>(OwnerComp.GetAIOwner());
	if (!Provider || !Provider->AdvanceToNextPatrolPoint())
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::Succeeded;
}

FString UWxBTTask_AdvancePatrol::GetStaticDescription() const
{
	return TEXT("정찰 경로의 다음 지점으로 진행");
}
