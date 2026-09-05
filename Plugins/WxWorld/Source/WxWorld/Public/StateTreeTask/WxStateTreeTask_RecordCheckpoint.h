// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Device/WxDeviceComponentName.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_RecordCheckpoint.generated.h"

USTRUCT()
struct FWxStateTreeTask_RecordCheckpointInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FWxStateTreeComponentName RespawnPoint;
};

/** 상호작용에 따른 라이브 상태 진입에서만 부활 지점을 갱신한다. 시작·복원은 기존 기록을 유지한다. */
USTRUCT(meta = (DisplayName = "체크포인트 기록", Category = "Wx"))
struct FWxStateTreeTask_RecordCheckpoint : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_RecordCheckpointInstanceData;
	FWxStateTreeTask_RecordCheckpoint();

	// StateTree GetInstanceDataType의 헤더 정의는 프로젝트 규칙의 예외다.
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
