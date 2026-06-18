// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxElevatorStateTreeNodes.h"

#include "Gimmick/WxElevator.h"
#include "StateTreeExecutionContext.h"

// ── ElevatorMove ───────────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_ElevatorMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AWxElevator* Elevator = Cast<AWxElevator>(Context.GetOwner());
	if (!Elevator)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 선택 시점의 권위 State 를 기록한다. 이후 State 가 이 값에서 벗어나면 Succeeded 로 완료한다.
	Instance.EnteredState = Elevator->GetElevatorState();

	const float TargetDistance = Elevator->GetTargetDistance();

	// 이미 목표 거리거나(슬롯 복원/정지) 이동 시간이 0이면 즉시 스냅하고 서버는 다음 상태로 승급한다.
	// 라이브 이동은 현재≠목표이므로 아래를 건너뛰고 Tick 이 보간한다.
	if (Elevator->GetMoveDuration() <= 0.f || FMath::IsNearlyEqual(Elevator->GetPlatformDistance(), TargetDistance))
	{
		Elevator->SetPlatformDistance(TargetDistance);
		Elevator->SetElevatorState(Instance.PromoteToState);
	}

	// 서버가 막 승급했다면 State 가 바뀌어 즉시 완료된다(duration 0/복원 스냅).
	return Elevator->GetElevatorState() != Instance.EnteredState ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_ElevatorMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AWxElevator* Elevator = Cast<AWxElevator>(Context.GetOwner());
	if (!Elevator)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const float TargetDistance = Elevator->GetTargetDistance();
	const float CurrentDistance = Elevator->GetPlatformDistance();

	// 아직 목표가 아니면 보간하고, 도달하면 서버가 다음 상태로 승급한다.
	if (!FMath::IsNearlyEqual(CurrentDistance, TargetDistance))
	{
		const float Duration = Elevator->GetMoveDuration();
		const float Length = Elevator->GetSplineLength();
		// 속도는 "초당 스플라인 거리". 한 끝점→다른 끝점(=Length)을 Duration 초에 주파.
		const float Speed = Duration > 0.f ? Length / Duration : Length;
		const float NewDistance = FMath::FInterpConstantTo(CurrentDistance, TargetDistance, DeltaTime, Speed);
		Elevator->SetPlatformDistance(NewDistance);

		// 클라에서는 SetElevatorState 가 노옵이며 OnRep_State 로 다음 상태에 진입한다.
		if (FMath::IsNearlyEqual(NewDistance, TargetDistance))
		{
			Elevator->SetElevatorState(Instance.PromoteToState);
		}
	}

	// 권위 State 가 진입 시점에서 벗어나면 완료해 재선택을 유발한다.
	// 서버는 위 승급으로 같은 틱에, 클라는 복제로 State 가 바뀐 뒤 완료(서버가 전이를 게이팅).
	return Elevator->GetElevatorState() != Instance.EnteredState ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ElevatorMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const UEnum* StateEnum = StaticEnum<EWxElevatorState>();
	const FText PromoteText = StateEnum
		? StateEnum->GetDisplayNameTextByValue(static_cast<int64>(InstanceData->PromoteToState))
		: FText::GetEmpty();

	return FText::Format(INVTEXT("Wx Elevator Move (→ {0})"), PromoteText);
}
#endif

// ── ElevatorDoorPose ───────────────────────────────────────────────────────

EStateTreeRunStatus FWxStateTreeTask_ElevatorDoorPose::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AWxElevator* Elevator = Cast<AWxElevator>(Context.GetOwner());
	if (!Elevator)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 선택 시점의 권위 State 를 기록한다. 이후 State 가 이 값에서 벗어나면 Succeeded 로 완료한다.
	Instance.EnteredState = Elevator->GetElevatorState();

	const float TargetAlpha = Instance.bOpen ? 1.f : 0.f;

	// 이미 목표 포즈거나(복원/정지 hold) 길이가 0이면 즉시 스냅하고 서버는 State 를 승급한다(정지 상태는 자기상태라 노옵).
	// 라이브 전이는 현재≠목표이므로 아래를 건너뛰고 Tick 이 보간한다.
	if (Elevator->GetDoorAnimDuration() <= 0.f || FMath::IsNearlyEqual(Elevator->GetDoorOpenAlpha(), TargetAlpha))
	{
		Elevator->SetDoorOpenAlpha(TargetAlpha);
		Elevator->SetElevatorState(Instance.PromoteToState);
	}

	// 서버가 막 승급했다면 State 가 바뀌어 즉시 완료된다(전이 상태의 duration 0/복원 스냅).
	return Elevator->GetElevatorState() != Instance.EnteredState ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_ElevatorDoorPose::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AWxElevator* Elevator = Cast<AWxElevator>(Context.GetOwner());
	if (!Elevator)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const float TargetAlpha = Instance.bOpen ? 1.f : 0.f;
	const float CurrentAlpha = Elevator->GetDoorOpenAlpha();

	// 아직 목표가 아니면 보간하고, 도달하면 서버가 다음 상태로 승급한다(정지 상태는 자기상태 승급이라 노옵).
	if (!FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha))
	{
		const float Duration = Elevator->GetDoorAnimDuration();
		const float Speed = Duration > 0.f ? 1.f / Duration : 1.f;
		const float NewAlpha = FMath::FInterpConstantTo(CurrentAlpha, TargetAlpha, DeltaTime, Speed);
		Elevator->SetDoorOpenAlpha(NewAlpha);

		if (FMath::IsNearlyEqual(NewAlpha, TargetAlpha))
		{
			Elevator->SetElevatorState(Instance.PromoteToState);
		}
	}

	// 권위 State 가 진입 시점에서 벗어나면 완료해 재선택을 유발한다.
	// 서버는 위 승급(전이 상태)이나 상호작용(정지 상태)으로, 클라는 복제로 State 가 바뀐 뒤 완료(서버가 게이팅).
	return Elevator->GetElevatorState() != Instance.EnteredState ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ElevatorDoorPose::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const UEnum* StateEnum = StaticEnum<EWxElevatorState>();
	const FText PromoteText = StateEnum
		? StateEnum->GetDisplayNameTextByValue(static_cast<int64>(InstanceData->PromoteToState))
		: FText::GetEmpty();

	return FText::Format(INVTEXT("Wx Elevator Door Pose ({0} → {1})"),
		InstanceData->bOpen ? INVTEXT("Open") : INVTEXT("Closed"),
		PromoteText);
}
#endif

// ── ElevatorInteraction ────────────────────────────────────────────────────

FWxStateTreeTask_ElevatorInteraction::FWxStateTreeTask_ElevatorInteraction()
{
	// 인터랙션을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_ElevatorInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AWxElevator* Elevator = Cast<AWxElevator>(Context.GetOwner()))
	{
		Elevator->SetAllInteractionsEnabled(Instance.bEnableInteraction);
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ElevatorInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wx Elevator Interaction ({0})"),
		InstanceData->bEnableInteraction ? INVTEXT("enabled") : INVTEXT("disabled"));
}
#endif

// ── ElevatorStateIs ────────────────────────────────────────────────────────

bool FWxStateTreeCondition_ElevatorStateIs::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const AWxElevator* Elevator = Cast<AWxElevator>(Context.GetOwner());
	return Elevator != nullptr && Elevator->GetElevatorState() == Instance.State;
}

#if WITH_EDITOR
FText FWxStateTreeCondition_ElevatorStateIs::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const UEnum* StateEnum = StaticEnum<EWxElevatorState>();
	const FText StateText = StateEnum
		? StateEnum->GetDisplayNameTextByValue(static_cast<int64>(InstanceData->State))
		: FText::GetEmpty();

	return FText::Format(INVTEXT("Wx Elevator State Is {0}"), StateText);
}
#endif
