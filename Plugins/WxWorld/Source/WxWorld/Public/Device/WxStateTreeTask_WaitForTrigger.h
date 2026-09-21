// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceTriggerRule.h"
#include "StateTreeTaskBase.h"
#include "StructUtils/InstancedStruct.h"
#include "WxStateTreeTask_WaitForTrigger.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AWxDevice;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 3 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_WaitForTriggerInstanceData
{
	GENERATED_BODY()
};

/**
 * 오너 장치가 작동될 때까지 Running 으로 머물다, 작동되는 순간 Succeeded 로 완료한다('상호작용 대기' 와 같은 대기 패턴). 어느 상태로 갈지는 이 상태의 「성공 시」 전이가 정한다.
 * 이 노드가 활성인 동안만 장치가 작동을 받는다 — 그래서 이 장치를 미는 버튼의 잠금이 곧 「대기 노드가 활성인가」이고, 트리 디버거에 그대로 보인다.
 * 움직이는 동안 잠그려면 이 노드를 정지 상태(Idle 리프)에만 두고, 일회용으로 만들려면 도착 상태에 두지 않는다. 활성 경로에 하나만 둔다.
 *
 * 폴링하지 않는다 — 진입에 장치에 등록하고, 장치가 약한 실행 컨텍스트로 완료를 통보한다. 설정은 트리 틱 밖(스캔·수락 검증)에서도 읽히므로 전부 노드 프로퍼티다.
 */
USTRUCT(meta = (DisplayName = "작동 대기", Category = "Wx|장치"))
struct FWxStateTreeTask_WaitForTrigger : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitForTriggerInstanceData;

	FWxStateTreeTask_WaitForTrigger();

	/** Sender 가 지금 작동 신호를 보내면 받아 줄 선택지. 수락 규칙이 없으면 누구에게서든 그냥 받는다(문구 없는 선택지 하나). */
	void GetAcceptedOptions(const AWxDevice& Receiver, const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

	/** 기다리는 동안 플레이어 상호작용도 연다. 끄면 다른 장치(버튼·레버)의 작동만 받는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bPlayerInteraction = false;

	/** 표시할 HUD 프롬프트. 코드 폴백이 없으므로 비우면 문구 없이 표시된다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bPlayerInteraction", EditConditionHides))
	FText Prompt;

	/**
	 * 연결 장치 중 하나라도 지금 작동을 기다리고 있을 때만 상호작용을 연다(버튼·레버용).
	 * 받는 장치가 이동 중이거나 더 기다리지 않는 상태면 프롬프트가 사라진다 — 잠금을 받는 쪽 트리의 대기 노드 배치로 정한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "bPlayerInteraction", EditConditionHides))
	bool bOnlyWhenLinkedAccepts = false;

	/** 누구에게서 무엇을 받아 줄지. 비워 두면 누가 보냈든 그냥 받는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ExcludeBaseStruct))
	TInstancedStruct<FWxDeviceTriggerRule> Rule;
};
