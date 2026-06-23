// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxRewardStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// ── GrantReward: 라이브 진입 시 권위 측에서 보상 지급 ──────────────────────────

USTRUCT()
struct FWxStateTreeTask_GrantRewardInstanceData
{
	GENERATED_BODY()

	// 바인딩할 파라미터가 없다 — RewardComponent 는 오너에서 자동 탐색, 직접 지급 대상은 항상 로컬 플레이어다.
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 Context 오너의 UWxRewardComponent(자동 탐색)의 DropRewards 를 호출하고 Succeeded 로 완료한다(1회성 보상 지급).
 * 아이템 타입별 분기(Pickup Fragment 있으면 월드 드랍, 없으면 로컬 플레이어(0번 컨트롤러) 인벤토리 직접 지급)는 DropRewards 가 처리하므로 여기선 트리거만 한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 호출하지 않는다 — 보상은 발동 순간에만 지급하고 복원/조인 시 중복 지급하지 않는다.
 * 보상 스폰/지급은 서버 권위 사건이라 클라 진입은 노옵(클라는 복제로 픽업/인벤토리를 추종). 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Wx Grant Reward"))
struct FWxStateTreeTask_GrantReward : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_GrantRewardInstanceData;

	FWxStateTreeTask_GrantReward();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
