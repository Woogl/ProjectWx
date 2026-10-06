// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceComponentName.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SplineMove.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class USceneComponent;
class USplineComponent;


USTRUCT()
struct FWxStateTreeTask_SplineMoveInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxStateTreeComponentName TargetComponent;

	/** 컴포넌트는 이 경로 위를 탄다고 가정한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedClasses = "/Script/Engine.SplineComponent"))
	FWxStateTreeComponentName Spline;

	/** 포인트 수를 넘으면 클램프, 음수면 움직이지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 TargetPointIndex = 0;

	/** 목표 포인트까지 주파 시간(초)으로, 0 이하면 즉시 스냅하고 이동 중 재진입하면 남은 거리를 이 시간에 주파한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 1.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> Component;

	UPROPERTY()
	TObjectPtr<USplineComponent> SplineComponent;

	UPROPERTY()
	float CurrentDistance = 0.f;

	UPROPERTY()
	float TargetDistance = 0.f;

	/** 시작→목표 구간의 일정 속도(초당 스플라인 거리). 시작점이 동적이라 EnterState 에서 1회 산출한다. */
	UPROPERTY()
	float MoveSpeed = 0.f;
};

/** 복원 진입이면 목표 포인트로 즉시 스냅하고, 그 밖의 진입에서는 컴포넌트의 실제 현재 위치에서 곡선을 따라 슬라이드한다. */
USTRUCT(meta = (DisplayName = "Spline Move", Category = "Wx"))
struct FWxStateTreeTask_SplineMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SplineMoveInstanceData;

	FWxStateTreeTask_SplineMove();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
