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
 * 라이브 전이로 진입할 때 권위 측에서 오너의 LinkedDevices 를 작동시킨다. 저작할 값이 없다 — 누구에게 보낼지는 배치(LinkedDevices)가, 받을지와 어느 상태로 갈지는 받는 장치의 대기 태스크와 그 상태의 「성공 시」 전이가 정한다.
 * 오너가 받은 당사자와 선택지 값을 그대로 넘기므로, 받는 장치는 누가 무엇을 골라 보냈는지로 목적지를 정할 수 있다(엘리베이터).
 *
 * 복원 진입이면 보내지 않는다 — 대상도 자기 복원 경로로 같은 상태에 수렴한다.
 */
USTRUCT(meta = (DisplayName = "연결 장치 작동", Category = "Wx|장치"))
struct FWxStateTreeTask_TriggerLinkedDevices : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_TriggerLinkedDevicesInstanceData;

	FWxStateTreeTask_TriggerLinkedDevices();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
