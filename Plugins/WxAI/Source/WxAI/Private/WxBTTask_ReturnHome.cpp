// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ReturnHome.h"

#include "WxAIPerceptionComponent.h"
#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_ReturnHome::UWxBTTask_ReturnHome()
{
	NodeName = TEXT("Return Home");

	// 에디터에서 별도 선택 없이 동작하도록 기본 키를 채워 둔다. 실제 키 해석은 InitializeFromAsset 가 한다.
	BlackboardKey.SelectedKeyName = WxBlackboardKeys::HomeLocation;
}

EBTNodeResult::Type UWxBTTask_ReturnHome::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// MoveTo 는 경로가 없거나 목표가 내비메시 밖이면 이동을 시작하지 못하고 동기 Failed 를 반환한다.
	// 복귀가 시작되지 않은 실패 경로에서는 현재 타겟도 잊지 않는다.
	const EBTNodeResult::Type MoveResult = Super::ExecuteTask(OwnerComp, NodeMemory);
	if (MoveResult != EBTNodeResult::InProgress)
	{
		return MoveResult;
	}

	// 타겟의 Perception 기록과 적용 상태 변경은 퍼셉션이 단일 지점에서 수행한다.
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
		{
			Perception->ForgetTargetActor();
		}
	}

	return MoveResult;
}
