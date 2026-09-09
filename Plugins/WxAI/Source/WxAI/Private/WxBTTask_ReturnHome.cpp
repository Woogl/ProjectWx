// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ReturnHome.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"

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

	// 감지 기록까지 지워야 타겟이 풀린다 — 블랙보드만 비우면 여전히 감지 중이라 선정 서비스가 곧바로 같은 대상을 다시 문다.
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (AIController && Blackboard)
	{
		if (UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent())
		{
			Perception->ForgetActor(WxBlackboardKeys::GetTargetActor(Blackboard));
		}

		WxBlackboardKeys::SetTargetActor(Blackboard, nullptr);
	}

	return MoveResult;
}
