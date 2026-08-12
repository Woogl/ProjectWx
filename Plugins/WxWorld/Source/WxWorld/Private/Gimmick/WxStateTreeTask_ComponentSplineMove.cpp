// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_ComponentSplineMove.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

FWxStateTreeTask_ComponentSplineMove::FWxStateTreeTask_ComponentSplineMove()
{
	// ComponentMove 와 같은 이유 — 목표 끝점을 선언하는 상태형이라, 재선택으로 다시 진입하면 주파 중이던 구간이 끊긴다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_ComponentSplineMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	const USplineComponent* Spline = Instance.Spline;
	if (!Component || !Spline)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 포인트가 없으면 목표할 위치가 없으므로 할 일 없이 곧바로 완료한다.
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints == 0)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 이 상태가 선언한 목표 포인트의 거리. State 가 끝점을 직접 가리키므로 초기 진입에서도 목적지를 안다.
	const int32 TargetIndex = FMath::Clamp(Instance.TargetPointIndex, 0, NumPoints - 1);
	const float TargetDistance = Spline->GetDistanceAlongSplineAtSplinePoint(TargetIndex);
	Instance.TargetDistance = TargetDistance;

	// 플랫폼의 실제 현재 위치에 해당하는 스플라인 거리를 시작점으로 잡는다(vertex 로 양자화하지 않아 이동 중 반전도 스냅 없이 현재 지점에서 출발).
	const float StartDistance = Spline->GetDistanceAlongSplineAtLocation(Component->GetComponentLocation(), ESplineCoordinateSpace::World);

	// 속도는 시작→목표 남은 거리/Duration 으로 EnterState 에서 1회 산출한다(Duration 0 이하면 아래에서 즉시 스냅).
	const float SegmentLength = FMath::Abs(TargetDistance - StartDistance);
	Instance.MoveSpeed = Instance.Duration > 0.f ? SegmentLength / Instance.Duration : SegmentLength;

	// 길이 0 이거나 이미 목표면 즉시 목표로 스냅, 아니면 시작에서 Tick 이 슬라이드한다.
	const bool bSnap = Instance.Duration <= 0.f || FMath::IsNearlyEqual(StartDistance, TargetDistance);
	Instance.CurrentDistance = bSnap ? TargetDistance : StartDistance;
	Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(Instance.CurrentDistance, ESplineCoordinateSpace::World));

	// 스냅으로 이미 도달했으면 곧바로 완료, 아니면 Tick 이 슬라이드하다 도달 시 완료한다.
	if (bSnap)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_ComponentSplineMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.TargetComponent;
	const USplineComponent* Spline = Instance.Spline;
	if (!Component || !Spline)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 도달 전까지 일정 속도로 스플라인 거리를 보간해 곡선을 따라 이동한다.
	if (!FMath::IsNearlyEqual(Instance.CurrentDistance, Instance.TargetDistance))
	{
		Instance.CurrentDistance = FMath::FInterpConstantTo(Instance.CurrentDistance, Instance.TargetDistance, DeltaTime, Instance.MoveSpeed);
		Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(Instance.CurrentDistance, ESplineCoordinateSpace::World));
	}

	// 도달하면 상태를 완료시킨다.
	const bool bReached = FMath::IsNearlyEqual(Instance.CurrentDistance, Instance.TargetDistance);
	return bReached ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_ComponentSplineMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 스플라인은 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText SplineText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Spline)), Formatting);
	if (SplineText.IsEmpty())
	{
		SplineText = InstanceData->Spline ? FText::FromString(InstanceData->Spline->GetName()) : INVTEXT("none");
	}

	return FText::Format(INVTEXT("Component Spline Move ({0} → point {1})"), SplineText, FText::AsNumber(InstanceData->TargetPointIndex));
}
#endif
