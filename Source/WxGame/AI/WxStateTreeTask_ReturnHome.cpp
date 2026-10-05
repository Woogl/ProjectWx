// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_ReturnHome.h"

#include "AI/WxAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_ReturnHome::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AWxAIController* AIController = Cast<AWxAIController>(Instance.AIController);
	if (!AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	Instance.Destination = AIController->GetHomeLocation();
	Instance.TargetActor = nullptr;

	// Move To 는 경로가 없거나 목표가 내비메시 밖이면 이동을 시작하지 못하고 그 자리에서 Failed 를 돌려준다.
	const EStateTreeRunStatus MoveStatus = Super::EnterState(Context, Transition);
	if (MoveStatus != EStateTreeRunStatus::Running)
	{
		return MoveStatus;
	}

	// 감지 기록까지 지워야 타겟이 풀린다 — 대상만 비우면 여전히 감지 중이라 평가자가 곧바로 같은 대상을 다시 문다.
	if (UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent())
	{
		Perception->ForgetActor(AIController->GetTargetActor());
	}

	AIController->SetTargetActor(nullptr);

	return MoveStatus;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ReturnHome::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return INVTEXT("홈 복귀");
}
#endif
