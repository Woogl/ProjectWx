// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_TriggerLinkedDevices.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;


USTRUCT()
struct FWxStateTreeTask_TriggerLinkedDevicesInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 오너의 당사자와 선택지 값을 그대로 넘겨 LinkedDevices 를 작동시킨다.
 *
 * 복원 진입이면 보내지 않는다 — 대상도 자기 복원 경로로 같은 상태에 수렴한다.
 */
USTRUCT(meta = (DisplayName = "Trigger Linked Devices", Category = "Wx|Device"))
struct FWxStateTreeTask_TriggerLinkedDevices : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_TriggerLinkedDevicesInstanceData;

	FWxStateTreeTask_TriggerLinkedDevices();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
