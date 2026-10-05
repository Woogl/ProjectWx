// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "GameplayTagContainer.h"
#include "WxStateTreeCondition_MasterAbilityActive.generated.h"

struct FStateTreeExecutionContext;


USTRUCT()
struct FWxStateTreeCondition_MasterAbilityActiveInstanceData
{
	GENERATED_BODY()

	/** 어빌리티 에셋 태그가 이 중 하나에 속하면(부모 태그 포함) 관찰 대상이다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Ability"))
	FGameplayTagContainer AbilityTags;
};

/**
 * 이 폰을 소환한 주인이 태그에 맞는 어빌리티를 실행 중이면 통과한다. 주인 없이 태어난 폰은 통과하지 못한다.
 * 평가할 때마다 주인의 어빌리티 목록을 훑으므로, 발동을 놓치지 않으려면 틱 전이에 건다.
 */
USTRUCT(meta = (DisplayName = "주인 어빌리티 실행 중", Category = "Wx"))
struct FWxStateTreeCondition_MasterAbilityActive : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeCondition_MasterAbilityActiveInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
