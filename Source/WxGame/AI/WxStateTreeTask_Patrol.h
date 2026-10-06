// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Tasks/StateTreeMoveToTask.h"
#include "WxStateTreeTask_Patrol.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;


USTRUCT()
struct FWxStateTreeTask_PatrolInstanceData : public FStateTreeMoveToTaskInstanceData
{
	GENERATED_BODY()

	/** 최대 이동 속도가 이 비율로 제한된다. (1.0 = 평상시 속도) */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MoveSpeedMultiplier = 0.5f;

	UPROPERTY()
	FActiveGameplayEffectHandle MoveSpeedEffectHandle;
};

/**
 * 엔진 Move To 를 상속해 이동/도착 판정/경로 실패·중단 처리는 엔진에 맡기고, 도착(성공)했을 때만 정찰 커서를 한 칸 진행한다.
 * 정찰 경로와 커서는 폰의 UWxAIBehaviorComponent 가 든다 — 중단되면 커서가 남아 다음 진입에서 이어서 정찰한다.
 * 갈 지점이 없으면(경로 없음, Once 완주) Failed 로 완료하므로, 그 폰이 머물 상태는 트리가 실패 전이로 정한다.
 *
 * 이동 목표(Destination·TargetActor)는 진입 때 정찰 지점으로 덮어쓰므로 저작 대상이 아니다.
 */
USTRUCT(meta = (DisplayName = "Patrol", Category = "Wx"))
struct FWxStateTreeTask_Patrol : public FStateTreeMoveToTask
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PatrolInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void StateCompleted(FStateTreeExecutionContext& Context, const EStateTreeRunStatus CompletionStatus, const FStateTreeActiveStates& CompletedActiveStates) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
