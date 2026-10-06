// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_GiveRewards.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;


USTRUCT()
struct FWxStateTreeTask_GiveRewardsInstanceData
{
	GENERATED_BODY()

	/** 비우면 아무것도 지급하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxGame.WxRewardTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle RewardRow;

	/** 드랍이 바닥에 끼지 않게 픽업 스폰 원점을 오너 기준으로 올리는 로컬 오프셋(cm)이며, 비-픽업(재화 등) 보상엔 영향 없다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector SpawnOffset = FVector(0.f, 0.f, 90.f);

	/** 픽업 발사 속도 벡터(월드 기준, cm/s). */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector LaunchVelocity = FVector(0.f, 0.f, 300.f);
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 UWxRewardLibrary::GrantReward 로 RewardRow 의 보상을 지급한다.
 * 장치 트리의 초기 진입·복원·레이트조인은 IsRestoring으로 제외한다. 퀘스트 등 일반 트리의 첫 진입은 지급한다.
 */
USTRUCT(meta = (DisplayName = "Give Rewards", Category = "Wx"))
struct FWxStateTreeTask_GiveRewards : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_GiveRewardsInstanceData;

	FWxStateTreeTask_GiveRewards();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
