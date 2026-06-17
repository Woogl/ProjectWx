// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoorStateTreeNodes.h"

#include "Gimmick/WxDoor.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_DoorPose::FWxStateTreeTask_DoorPose()
{
	// 정적 포즈 유지 태스크는 틱이 불필요하다(전이는 이벤트/완료로 발생).
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_DoorPose::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner()))
	{
		Door->SetConsoleInteractionEnabled(Instance.bEnableInteraction);
		Door->SetDoorOpenAlpha(Instance.OpenAlpha);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_DoorOpening::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.Elapsed = 0.f;

	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	Door->SetConsoleInteractionEnabled(false);
	Door->SetDoorOpenAlpha(0.f);

	// 길이가 0 이하이면 한 프레임도 기다리지 않고 즉시 완전 개방 완료.
	if (Door->GetDoorAnimDuration() <= 0.f)
	{
		Door->SetDoorOpenAlpha(1.f);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_DoorOpening::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.Elapsed += DeltaTime;

	const float Duration = Door->GetDoorAnimDuration();
	const float Alpha = Duration > 0.f ? FMath::Clamp(Instance.Elapsed / Duration, 0.f, 1.f) : 1.f;
	Door->SetDoorOpenAlpha(Alpha);

	return Alpha >= 1.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

bool FWxStateTreeCondition_DoorTriggered::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	return Door != nullptr && Door->IsDoorTriggered();
}
