// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_StartNextQuest.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UStateTree;


USTRUCT()
struct FWxStateTreeTask_StartNextQuestInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSoftObjectPtr<UStateTree> Quest;
};

/**
 * 진입 시 다음 퀘스트 시작을 오너의 퀘스트 컴포넌트에 예약하고 즉시 Succeeded 로 완료한다.
 * 러너 실행 콜스택 안에서는 에셋 교체가 거부되므로 즉시 시작이 아니라 다음 틱 예약이다.
 * 퀘스트 컴포넌트가 없으면 잘못된 조립(퀘스트 러너 밖 사용)이라 예약 없이 Failed 로 끝난다.
 */
USTRUCT(meta = (DisplayName = "Start Next Quest", Category = "Wx"))
struct FWxStateTreeTask_StartNextQuest : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_StartNextQuestInstanceData;

	FWxStateTreeTask_StartNextQuest();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
