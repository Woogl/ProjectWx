// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_AdvancePatrol.generated.h"

/**
 * BT Task: 정찰 커서를 다음 지점으로 진행시킨다.
 *
 * AIController 가 IWxPatrolRouteProvider 를 구현해야 하며, 진행 결과는 Blackboard 의 PatrolTargetLocation 으로 발행된다.
 * 보통 [MoveTo(PatrolTargetLocation) → Wait → AdvancePatrol] 시퀀스의 마지막에 배치해, 한 지점 도착 후 다음 지점으로 넘긴다.
 */
UCLASS()
class WXAI_API UWxBTTask_AdvancePatrol : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWxBTTask_AdvancePatrol();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;
};
