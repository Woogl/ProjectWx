// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_ComponentSplineMove.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class USceneComponent;
class USplineComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_ComponentSplineMoveInstanceData
{
	GENERATED_BODY()

	/** 스플라인 위를 움직일 씬 컴포넌트. ST 에셋에서 Context 액터의 컴포넌트로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USceneComponent> TargetComponent;

	/** 경로를 정의하는 스플라인. 컴포넌트는 이 경로 위를 탄다고 가정한다. ST 에셋에서 Context 액터의 스플라인으로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USplineComponent> Spline;

	/** 이 상태가 목표하는 스플라인 포인트 인덱스. 각 상태가 자기 끝점을 직접 선언한다(초기 진입 스냅·라이브 슬라이드의 목적지). 범위를 벗어나면 클램프. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	int32 TargetPointIndex = 0;

	/** 목표 포인트까지 주파 시간(초). 0 이하면 즉시 스냅. 속도는 시작→목표 남은 거리/Duration 이라, 이동 중 재진입 시엔 남은 거리를 이 시간에 주파한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 1.f;

	/** (런타임) Tick 이 보간하는 현재 스플라인 거리. EnterState 에서 시작 거리(현재 위치의 스플라인 거리)로 초기화한다. */
	UPROPERTY()
	float CurrentDistance = 0.f;

	/** (런타임) 목표 포인트(TargetPointIndex)의 스플라인 거리. EnterState 에서 캡처한다. */
	UPROPERTY()
	float TargetDistance = 0.f;

	/** (런타임) 시작→목표 구간의 일정 속도(초당 스플라인 거리). EnterState 에서 1회 산출한다(시작점은 동적이라 Tick 이 고정 저자값에서 재계산 불가). */
	UPROPERTY()
	float MoveSpeed = 0.f;
};

/**
 * 지정 컴포넌트를 TargetPointIndex 가 가리키는 스플라인 포인트로 옮기고, 도달하면 Succeeded 를 반환해 상태를 완료시킨다.
 * 각 상태가 자기 목표 끝점을 직접 선언하는 순수 비주얼 태스크라 어떤 기믹이든 경로 이동에 재사용한다.
 * 진입 경로를 가리지 않고 플랫폼의 실제 현재 위치에서 목표 포인트까지 곡선을 따라 슬라이드한다(Duration 0 이하·이미 목표면 즉시 스냅). 이동 중 재진입해도 vertex 로 스냅하지 않고 현재 지점에서 반전한다.
 */
USTRUCT(meta = (DisplayName = "컴포넌트 스플라인 이동", Category = "Wx"))
struct FWxStateTreeTask_ComponentSplineMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ComponentSplineMoveInstanceData;

	FWxStateTreeTask_ComponentSplineMove();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
