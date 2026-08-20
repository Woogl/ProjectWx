// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeDelegate.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_EnableDeviceInteraction.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_EnableDeviceInteractionInstanceData
{
	GENERATED_BODY()

	/** 오너 기믹의 장치 링크에서 대상을 고르는 키(수신자 관점의 명령형 동사). 링크 배선은 인스턴스 저작이라 공유 에셋은 이 이름만 안다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Role = TEXT("Activate");

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = false;

	/** 켜진 장치가 표시할 HUD 프롬프트. 비우면 장치의 저작 기본 문구가 쓰인다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bEnable"))
	FText Prompt;

	/** 이 역할의 장치가 눌렸을 때 오너 기믹이 발행하는 델리게이트. 전이의 Delegate 칸에서 이것을 골라 목적지를 잇는다(끄는 노드의 것은 발행될 일이 없다). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FStateTreeDelegateDispatcher OnInteracted;
};

/**
 * 진입 시 오너 기믹에 링크된 역할의 레버 장치를 bEnable 로 켜고 끄며 완료한다.
 * 상태를 떠나도 되돌리지 않으므로, 다시 열 시점은 여는 상태에 이 태스크를 한 번 더 두어 에셋이 정한다.
 * 같은 역할에 장치가 여럿이면(N:1) 전부 같은 상태로 움직이고, 어느 것이 눌려도 같은 발행자가 발행된다.
 * 이 노드의 OnInteracted 를 지목하는 전이는 이 노드가 있는 상태나 그 하위 상태에 두어야 한다 — 부모 상태의 전이는 자식의 발행자를 지목하지 못한다.
 */
USTRUCT(meta = (DisplayName = "장치 상호작용 켜기", Category = "Wx"))
struct FWxStateTreeTask_EnableDeviceInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableDeviceInteractionInstanceData;

	FWxStateTreeTask_EnableDeviceInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
