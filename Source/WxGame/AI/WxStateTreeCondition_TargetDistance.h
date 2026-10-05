// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "StateTreeTypes.h"
#include "WxStateTreeCondition_TargetDistance.generated.h"

struct FStateTreeExecutionContext;


USTRUCT()
struct FWxStateTreeCondition_TargetDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", ForceUnits = "cm"))
	float Distance = 0.f;
};

/**
 * 폰과 AWxAIController 가 겨누는 대상 사이의 거리를 평가하는 순간에 재서 비교한다.
 * 대상이 없으면 무한히 먼 것으로 본다 — 0으로 두면 근거리 비교를 통과해 버린다.
 */
USTRUCT(meta = (DisplayName = "타겟 거리 비교", Category = "Wx"))
struct FWxStateTreeCondition_TargetDistance : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_TargetDistanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Parameter")
	UE::StateTree::EComparisonOperator Operator = UE::StateTree::EComparisonOperator::LessOrEqual;
};
