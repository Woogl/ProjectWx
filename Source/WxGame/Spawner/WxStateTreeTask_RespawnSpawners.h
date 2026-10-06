// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_RespawnSpawners.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;


USTRUCT()
struct FWxStateTreeTask_RespawnSpawnersInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 월드의 Auto 모드 스포너를 일괄 리스폰한다(체크포인트 휴식 시 적 리스폰).
 * 장치 트리의 시작·복원(레이트조인·장치 복원 전이)이면 호출하지 않는다 — 리스폰은 발동 순간의 효과다.
 */
USTRUCT(meta = (DisplayName = "Respawn Spawners", Category = "Wx"))
struct FWxStateTreeTask_RespawnSpawners : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_RespawnSpawnersInstanceData;

	FWxStateTreeTask_RespawnSpawners();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const override;
#endif
};
