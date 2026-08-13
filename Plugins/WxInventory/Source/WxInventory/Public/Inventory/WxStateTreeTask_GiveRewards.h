// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_GiveRewards.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_GiveRewardsInstanceData
{
	GENERATED_BODY()

	/** 비우면 아무것도 지급하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (RowType = "/Script/WxInventory.WxRewardTableRow"))
	FDataTableRowHandle RewardRow;

	/**
	 * 픽업 스폰 원점을 오너 기준으로 올리는 로컬 오프셋(cm) — 드랍이 바닥에 끼지 않게 한다.
	 * 비-픽업(재화 등) 보상엔 영향 없다.
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector SpawnOffset = FVector(0.f, 0.f, 90.f);

	/** 픽업 발사 속도 벡터(월드 기준, cm/s) — 방향과 크기를 모두 담는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector LaunchVelocity = FVector(0.f, 0.f, 300.f);
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 UWxRewardLibrary::GrantReward 로 인스턴스 데이터(RewardRow)의 보상을 지급하고 Succeeded 로 완료한다(1회성 보상 지급).
 * 아이템 타입별 분기(Pickup Fragment 있으면 월드 드랍, 없으면 로컬 플레이어(0번 컨트롤러) 인벤토리 직접 지급)는 GrantReward 가 처리하므로 여기선 트리거만 한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 호출하지 않는다 — 보상은 발동 순간에만 지급하고 복원/조인 시 중복 지급하지 않는다.
 * 보상 스폰/지급은 서버 권위 사건이라 클라 진입은 노옵(클라는 복제로 픽업/인벤토리를 추종).
 */
USTRUCT(meta = (DisplayName = "보상 지급", Category = "Wx"))
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
