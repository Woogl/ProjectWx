// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxStateTreeComponentName.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_ComponentMove.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class USceneComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_ComponentMoveInstanceData
{
	GENERATED_BODY()

	/** 트리가 붙은 액터가 가진 컴포넌트 중에서 고른다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxStateTreeComponentName TargetComponent;

	/** 아키타입 대비 목표 상대 좌표 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector LocalOffset = FVector::ZeroVector;

	/** 목표까지 슬라이드 시간(초). 0 이하면 즉시 스냅. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Duration = 1.f;

	/** (런타임) 지목이 가리키는 컴포넌트. */
	UPROPERTY()
	TObjectPtr<USceneComponent> Component;

	/** (런타임) 시작→목표 구간의 일정 속도(초당 로컬 거리). */
	UPROPERTY()
	float MoveSpeed = 0.f;

	/** (런타임) 계산된 목표 상대 위치. */
	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;
};

/**
 * 지정 컴포넌트를 현재 상대 위치에서 기준(아키타입)+LocalOffset 으로 일정 속도 슬라이드하고, 도달하면 Succeeded 를 반환해 상태를 완료시킨다.
 * 속도를 시작→목표 실제 거리에서 산출하므로 목표가 아키타입(offset 0)인 '닫기' 방향도 일정 속도가 된다.
 */
USTRUCT(meta = (DisplayName = "컴포넌트 이동", Category = "Wx"))
struct FWxStateTreeTask_ComponentMove : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_ComponentMoveInstanceData;

	FWxStateTreeTask_ComponentMove();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
