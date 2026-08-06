// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ReturnHome.h"

#include "WxAIPerceptionComponent.h"
#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UWxBTTask_ReturnHome::UWxBTTask_ReturnHome()
{
	NodeName = TEXT("Return Home");

	// 기본 이동 목표 키를 HomeLocation 으로 지정한다(에디터에서 별도 선택 없이 동작). 실제 키 해석은 InitializeFromAsset 가 한다.
	BlackboardKey.SelectedKeyName = WxBlackboardKeys::HomeLocation;

	// 종료 시 억제를 해제하기 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UWxBTTask_ReturnHome::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	// 퍼셉션에 억제를 지시해 타겟/인식/회전을 원복하고 복귀 중 재감지를 막는다. 상태 변경은 퍼셉션이 단일 지점에서 수행한다.
	if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
	{
		Perception->SetTargetingSuppressed(true);
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

void UWxBTTask_ReturnHome::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
	
	// 복귀가 끝났으니 억제를 해제한다. 리시 안으로 돌아왔으면 다음 자극에서 다시 정상 감지·전투한다.
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
		{
			Perception->SetTargetingSuppressed(false);
		}
	}
}
