// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeDelegate.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_EnableInteraction.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_EnableInteractionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = false;

	/** 표시할 HUD 프롬프트. 코드 폴백이 없으므로 비우면 문구 없이 표시된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bEnable", EditConditionHides))
	FText Prompt;

	/** 눌렸을 때 오너 장치가 발행한다(끄는 노드의 것은 발행될 일이 없다). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FStateTreeDelegateDispatcher OnInteracted;
};

/**
 * 진입 시 오너 장치의 상호작용을 bEnable 로 켜거나 끄고 완료한다. 켤 때는 프롬프트와 발행 자리를 그 장치에 함께 담는다.
 * 상태를 떠나도 되돌리지 않는다.
 *
 * 이 노드의 OnInteracted 를 지목하는 전이는 이 노드가 있는 상태나 그 하위 상태에 두어야 한다 — 바인딩이 볼 수 있는 범위가 루트에서 전이가 달린 상태까지의 경로뿐이라, 부모 상태의 전이는 자식의 발행자를 지목하지 못한다.
 *
 * 오너 자신만 여닫는다. 남의 장치를 잠그는 것은 '이벤트 보내기' 로 그 트리에 상태를 요청하고, NPC 는 '상호작용 대기' 가 기다리는 동안 스스로 열린다.
 */
USTRUCT(meta = (DisplayName = "상호작용 켜기", Category = "Wx"))
struct FWxStateTreeTask_EnableInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableInteractionInstanceData;

	FWxStateTreeTask_EnableInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
