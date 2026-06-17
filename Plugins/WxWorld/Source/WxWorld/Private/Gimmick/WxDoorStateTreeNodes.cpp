// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoorStateTreeNodes.h"

#include "Gimmick/WxDoor.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_DoorPose::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float TargetAlpha = Instance.bOpen ? 1.f : 0.f;

	// 이미 목표 포즈거나(복원/레이트조인/정지 hold) 길이가 0이면 즉시 스냅하고 서버는 State 를 승급한다.
	// 라이브 전이는 현재≠목표이므로 아래를 건너뛰고 Tick 이 보간한다.
	if (Door->GetDoorAnimDuration() <= 0.f || FMath::IsNearlyEqual(Door->GetDoorOpenAlpha(), TargetAlpha))
	{
		Door->SetDoorOpenAlpha(TargetAlpha);
		Door->SetDoorState(Instance.bOpen ? EWxDoorState::Open : EWxDoorState::Closed);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_DoorPose::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const float TargetAlpha = Instance.bOpen ? 1.f : 0.f;
	const float CurrentAlpha = Door->GetDoorOpenAlpha();

	// 이미 목표면 더 보간할 것이 없다(EnterState 가 스냅·승급을 끝냈거나, 도달 후 재선택 대기).
	if (FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha))
	{
		return EStateTreeRunStatus::Running;
	}

	const float Duration = Door->GetDoorAnimDuration();
	const float Speed = Duration > 0.f ? 1.f / Duration : 1.f;
	const float NewAlpha = FMath::FInterpConstantTo(CurrentAlpha, TargetAlpha, DeltaTime, Speed);
	Door->SetDoorOpenAlpha(NewAlpha);

	// 목표 도달 시 서버가 State 를 승급한다. State 변경 → 재선택으로 이 태스크를 벗어난다.
	// 클라에서는 SetDoorState 가 노옵이며 OnRep_State 로 다음 상태에 진입한다.
	if (FMath::IsNearlyEqual(NewAlpha, TargetAlpha))
	{
		Door->SetDoorState(Instance.bOpen ? EWxDoorState::Open : EWxDoorState::Closed);
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_DoorPose::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Door Pose ({0})"), InstanceData->bOpen ? INVTEXT("Open") : INVTEXT("Closed"));
}
#endif

FWxStateTreeTask_DoorInteraction::FWxStateTreeTask_DoorInteraction()
{
	// 인터랙션을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_DoorInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner()))
	{
		Door->SetConsoleInteractionEnabled(Instance.bEnableInteraction);
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_DoorInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Door Interaction ({0})"),
		InstanceData->bEnableInteraction ? INVTEXT("enabled") : INVTEXT("disabled"));
}
#endif

bool FWxStateTreeCondition_DoorStateIs::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	return Door != nullptr && Door->GetDoorState() == Instance.State;
}

#if WITH_EDITOR
FText FWxStateTreeCondition_DoorStateIs::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const UEnum* StateEnum = StaticEnum<EWxDoorState>();
	const FText StateText = StateEnum
		? StateEnum->GetDisplayNameTextByValue(static_cast<int64>(InstanceData->State))
		: FText::GetEmpty();

	return FText::Format(INVTEXT("Wx Door State Is {0}"), StateText);
}
#endif
