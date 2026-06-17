// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoorStateTreeNodes.h"

#include "Gimmick/WxDoor.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_DoorPose::FWxStateTreeTask_DoorPose()
{
	// 정적 포즈 유지 태스크는 틱이 불필요하다(전이는 재선택으로 발생).
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_DoorPose::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner()))
	{
		Door->SetConsoleInteractionEnabled(Instance.bEnableInteraction);
		Door->SetDoorOpenAlpha(Instance.bOpen ? 1.f : 0.f);
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

	// 길이가 0 이하이면 즉시 완전 개방하고 서버는 Open 으로 승급. State 변경이 재선택을 일으켜 이 태스크를 벗어난다.
	if (Door->GetDoorAnimDuration() <= 0.f)
	{
		Door->SetDoorOpenAlpha(1.f);
		Door->SetDoorState(EWxDoorState::Open);
		return EStateTreeRunStatus::Running;
	}

	Door->SetDoorOpenAlpha(0.f);
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

	// 개방 완료 시 서버가 Open 으로 승급한다. State 변경 → 재선택으로 이 태스크를 벗어난다.
	// 클라에서는 SetDoorState 가 노옵이며 OnRep_State 로 Open 에 진입한다. 그때까지 완전 개방 포즈로 머문다.
	if (Alpha >= 1.f)
	{
		Door->SetDoorState(EWxDoorState::Open);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_DoorClosing::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.Elapsed = 0.f;

	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	Door->SetConsoleInteractionEnabled(false);

	// 길이가 0 이하이면 즉시 완전 폐쇄하고 서버는 Closed 로 승급. State 변경이 재선택을 일으켜 이 태스크를 벗어난다.
	if (Door->GetDoorAnimDuration() <= 0.f)
	{
		Door->SetDoorOpenAlpha(0.f);
		Door->SetDoorState(EWxDoorState::Closed);
		return EStateTreeRunStatus::Running;
	}

	Door->SetDoorOpenAlpha(1.f);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_DoorClosing::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.Elapsed += DeltaTime;

	const float Duration = Door->GetDoorAnimDuration();
	const float Alpha = Duration > 0.f ? FMath::Clamp(1.f - Instance.Elapsed / Duration, 0.f, 1.f) : 0.f;
	Door->SetDoorOpenAlpha(Alpha);

	// 폐쇄 완료 시 서버가 Closed 로 승급한다. State 변경 → 재선택으로 이 태스크를 벗어난다.
	// 클라에서는 SetDoorState 가 노옵이며 OnRep_State 로 Closed 에 진입한다. 그때까지 완전 폐쇄 포즈로 머문다.
	if (Alpha <= 0.f)
	{
		Door->SetDoorState(EWxDoorState::Closed);
	}

	return EStateTreeRunStatus::Running;
}

bool FWxStateTreeCondition_DoorStateIs::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	return Door != nullptr && Door->GetDoorState() == Instance.State;
}
