// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "WxStateTreeCondition_BeyondLeash.generated.h"

struct FStateTreeExecutionContext;

UENUM()
enum class EWxLeashAnchor : uint8
{
	/** 빙의한 순간 폰이 서 있던 자리. */
	Home,

	/** 이 폰을 소환한 주인. 주인 없이 태어난 폰은 이탈로 보지 않는다. */
	Master,
};

USTRUCT()
struct FWxStateTreeCondition_BeyondLeashInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", ForceUnits = "cm"))
	float LeashRadius = 3000.f;
};

/**
 * 폰이 앵커에서 LeashRadius 넘게 벗어났는지(리시 이탈) 판정한다.
 * 복귀를 언제 끝낼지는 이 조건이 아니라 복귀 상태의 태스크가 정한다 — 복귀 상태에는 이 조건으로 빠져나가는 전이를 두지 않는다.
 */
USTRUCT(meta = (DisplayName = "Beyond Leash", Category = "Wx"))
struct FWxStateTreeCondition_BeyondLeash : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_BeyondLeashInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EWxLeashAnchor Anchor = EWxLeashAnchor::Home;
};
