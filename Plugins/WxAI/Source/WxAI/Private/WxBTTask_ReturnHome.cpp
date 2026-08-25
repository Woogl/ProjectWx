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

	// 종료 시 억제를 해제하기 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UWxBTTask_ReturnHome::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// MoveTo 는 경로가 없거나 목표가 내비메시 밖이면 이동을 시작하지 못하고 동기 Failed 를 반환한다.
	// 그 헛발질에 타겟을 비우면, 해제 시 재획득이 지금 감지 중인 대상만 집으므로 시야 밖으로 빠진 타겟이 영영 사라진다.
	const EBTNodeResult::Type MoveResult = Super::ExecuteTask(OwnerComp, NodeMemory);
	if (MoveResult != EBTNodeResult::InProgress)
	{
		return MoveResult;
	}

	// 타겟/인식/회전 상태 변경은 퍼셉션이 단일 지점에서 수행한다.
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
		{
			Perception->SetTargetingSuppressed(true);
		}
	}

	return MoveResult;
}

void UWxBTTask_ReturnHome::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
	
	// 억제를 켠 적 없는 종료 경로에서도 그냥 부른다 — 전환에서만 동작하는 setter 라 no-op 이다.
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
		{
			Perception->SetTargetingSuppressed(false);
		}
	}
}
