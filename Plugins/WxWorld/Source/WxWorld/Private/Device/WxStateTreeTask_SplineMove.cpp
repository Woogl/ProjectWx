// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_SplineMove.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxWorldModule.h"

FWxStateTreeTask_SplineMove::FWxStateTreeTask_SplineMove()
{
	// ComponentMove 와 같은 이유 — 목표 끝점을 선언하는 상태형이라, 재선택으로 다시 진입하면 주파 중이던 구간이 끊긴다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_SplineMove::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	Instance.Component = Instance.TargetComponent.Resolve(Owner);
	Instance.SplineComponent = Cast<USplineComponent>(Instance.Spline.Resolve(Owner));

	USceneComponent* Component = Instance.Component;
	const USplineComponent* Spline = Instance.SplineComponent;
	if (!Component || !Spline)
	{
		UE_LOG(LogWxWorld, Error, TEXT("Spline Move: %s 에서 컴포넌트 '%s' 또는 스플라인 '%s' 를 찾지 못했다."), *GetNameSafe(Owner), *Instance.TargetComponent.Name.ToString(), *Instance.Spline.Name.ToString());
		return EStateTreeRunStatus::Failed;
	}

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints == 0)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const int32 TargetIndex = FMath::Clamp(Instance.TargetPointIndex, 0, NumPoints - 1);
	const float TargetDistance = Spline->GetDistanceAlongSplineAtSplinePoint(TargetIndex);
	Instance.TargetDistance = TargetDistance;

	// vertex 로 양자화하지 않아야 이동 중 반전도 스냅 없이 현재 지점에서 출발한다.
	const float StartDistance = Spline->GetDistanceAlongSplineAtLocation(Component->GetComponentLocation(), ESplineCoordinateSpace::World);

	const float SegmentLength = FMath::Abs(TargetDistance - StartDistance);
	Instance.MoveSpeed = Instance.Duration > 0.f ? SegmentLength / Instance.Duration : SegmentLength;

	const bool bSnap = Instance.Duration <= 0.f || FMath::IsNearlyEqual(StartDistance, TargetDistance);
	Instance.CurrentDistance = bSnap ? TargetDistance : StartDistance;
	Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(Instance.CurrentDistance, ESplineCoordinateSpace::World));

	if (bSnap)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_SplineMove::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USceneComponent* Component = Instance.Component;
	const USplineComponent* Spline = Instance.SplineComponent;
	if (!Component || !Spline)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!FMath::IsNearlyEqual(Instance.CurrentDistance, Instance.TargetDistance))
	{
		Instance.CurrentDistance = FMath::FInterpConstantTo(Instance.CurrentDistance, Instance.TargetDistance, DeltaTime, Instance.MoveSpeed);
		Component->SetWorldLocation(Spline->GetLocationAtDistanceAlongSpline(Instance.CurrentDistance, ESplineCoordinateSpace::World));
	}

	const bool bReached = FMath::IsNearlyEqual(Instance.CurrentDistance, Instance.TargetDistance);
	return bReached ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SplineMove::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FName SplineName = InstanceData->Spline.Name;
	return FText::Format(INVTEXT("스플라인 이동 ({0})"),
		SplineName.IsNone() ? INVTEXT("none") : FText::FromName(SplineName));
}
#endif
