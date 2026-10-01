// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_RefillItemCharges.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;


USTRUCT()
struct FWxStateTreeTask_RefillItemChargesInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 로컬 플레이어(0번 컨트롤러) 인벤토리의 전 아이템에 UWxInventoryComponent::RefillItemCharges 를 돌린다 — 충전형(Charges Fragment)이 아닌 아이템은 그 안에서 걸러진다.
 * 장치 트리의 초기 진입·복원·레이트조인은 IsRestoring으로 제외한다. 일반 트리의 첫 진입은 실행한다.
 */
USTRUCT(meta = (DisplayName = "아이템 충전 리필", Category = "Wx"))
struct FWxStateTreeTask_RefillItemCharges : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_RefillItemChargesInstanceData;

	FWxStateTreeTask_RefillItemCharges();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	// 대상이 로컬 플레이어 인벤토리로 고정이고 파라미터도 없어 GetDescription 으로 덧붙일 것이 없다 — 표시 이름은 DisplayName 메타가 그대로 쓰인다.
};
