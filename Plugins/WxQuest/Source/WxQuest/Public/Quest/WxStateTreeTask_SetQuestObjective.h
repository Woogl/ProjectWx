// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_SetQuestObjective.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_SetQuestObjectiveInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FText ObjectiveText;

	/** (런타임) 제거는 이 기록만 근거로 한다. */
	UPROPERTY()
	int32 ObjectiveHandle = INDEX_NONE;
};

/**
 * 진입 시 저널에 목표를 하나 걸고, 상태에 머무는 동안 유지하다 떠날 때 걷어간다.
 * 목표의 수명이 곧 그 상태의 수명이라 정리 태스크가 따로 필요 없다.
 * 병렬 상태가 없는 StateTree 에서 다중 목표는 부모 상태와 자식 상태가 각각 걸어 성립한다.
 * 진입 즉시 Succeeded 로 끝나며, 상태 완료는 짝이 되는 Wait 태스크가 낸다.
 *
 * 퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이다.
 * Failed 를 돌려주긴 하지만 이 태스크는 bConsideredForCompletion=false 라 엔진이 그 반환 상태를 결과에 반영하지 않는다 — 트리는 그대로 진행하며, 오조립은 경고 로그로만 드러난다.
 */
USTRUCT(meta = (DisplayName = "퀘스트 목표 설정", Category = "Wx"))
struct FWxStateTreeTask_SetQuestObjective : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_SetQuestObjectiveInstanceData;

	FWxStateTreeTask_SetQuestObjective();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
