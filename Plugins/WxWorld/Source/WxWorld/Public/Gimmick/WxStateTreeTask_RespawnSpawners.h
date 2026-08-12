// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_RespawnSpawners.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_RespawnSpawnersInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서 월드의 Auto 모드 스포너를 일괄 리스폰하고 Succeeded 로 완료한다(체크포인트 휴식 시 적 리스폰).
 * 대상을 지정하지 않고 월드 전체를 훑는다는 점에서 'Trigger Spawners'(지정 스포너 트리거)와 갈린다.
 * 초기 진입(StateTree 시작/복원/레이트조인)이면 호출하지 않는다 — 리스폰은 발동 순간의 효과다.
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
