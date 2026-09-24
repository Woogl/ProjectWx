// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_SplineMove.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "Device/WxDeviceStateTreeComponent.h"
#include "WxWorldModule.h"

FWxStateTreeTask_SplineMove::FWxStateTreeTask_SplineMove()
{
	// 재선택에도 다시 진입한다 — 목표가 바인딩(Actor.SelectedOptionValue)으로 바뀔 수 있어서다.
	// 클라가 이동을 보지 못한 채 같은 상태의 새 진입만 받으면(컬 거리 밖에 있다 돌아온 경우) 재선택이 되는데, 그때 새 목표로 다시 맞추지 않으면 탑승칸이 옛 자리에 남는다.
	// 대가로 주파 도중 재선택되면 남은 거리를 Duration 에 다시 주파한다.
	bShouldStateChangeOnReselect = true;
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

	// 음수는 「아직 목적지를 받은 적 없다」는 뜻이라(Actor.SelectedOptionValue 바인딩의 초기값) 배치된 자리에 그대로 둔다.
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints == 0 || Instance.TargetPointIndex < 0)
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

	const bool bSnap = UWxDeviceStateTreeComponent::IsRestoring(Context, Transition) || Instance.Duration <= 0.f || FMath::IsNearlyEqual(StartDistance, TargetDistance);
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
