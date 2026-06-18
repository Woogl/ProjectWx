// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxDoorStateTreeNodes.h"

#include "Gimmick/WxDoor.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_DoorPose::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AWxDoor* Door = Cast<AWxDoor>(Context.GetOwner());
	if (!Door)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 선택 시점의 권위 State 를 기록한다. 이후 State 가 이 값에서 벗어나면(상호작용) Tick 이 Succeeded 로 완료한다.
	Instance.EnteredState = Door->GetDoorState();

	const float TargetAlpha = Instance.bOpen ? 1.f : 0.f;

	// 이미 목표 포즈거나(복원/레이트조인/정지 hold) 길이가 0이면 즉시 스냅한다.
	// 라이브 전이는 현재≠목표이므로 아래를 건너뛰고 Tick 이 보간한다.
	if (Door->GetDoorAnimDuration() <= 0.f || FMath::IsNearlyEqual(Door->GetDoorOpenAlpha(), TargetAlpha))
	{
		Door->SetDoorOpenAlpha(TargetAlpha);
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

	// 권위 State 가 진입 시점에서 벗어나면(상호작용으로 반대 목표 확정) 완료해 재선택을 유발한다.
	// 서버는 SetDoorState 즉시, 클라는 복제로 State 가 바뀐 뒤 완료(서버가 전이를 게이팅).
	if (Door->GetDoorState() != Instance.EnteredState)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 목표 포즈로 보간하고, 도달하면 그대로 hold(이 상태가 안정 상태).
	const float TargetAlpha = Instance.bOpen ? 1.f : 0.f;
	const float CurrentAlpha = Door->GetDoorOpenAlpha();
	if (FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha))
	{
		return EStateTreeRunStatus::Running;
	}

	const float Duration = Door->GetDoorAnimDuration();
	const float Speed = Duration > 0.f ? 1.f / Duration : 1.f;
	const float NewAlpha = FMath::FInterpConstantTo(CurrentAlpha, TargetAlpha, DeltaTime, Speed);
	Door->SetDoorOpenAlpha(NewAlpha);

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
