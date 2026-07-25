// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxInventoryStateTreeNodes.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

// ── RefillItemCharges: 라이브 진입 시 권위 측에서 충전형 아이템 리필 ──────────

USTRUCT()
struct FWxStateTreeTask_RefillItemChargesInstanceData
{
	GENERATED_BODY()
};

/**
 * 라이브 전이로 진입할 때 권위 측에서만 로컬 플레이어(0번 컨트롤러) 인벤토리의 충전형(Charges Fragment) 아이템을 MaxCharges 까지 채우고 Succeeded 로 완료한다(체크포인트 에스트병 리필).
 * 충전형이 아닌 아이템은 UWxInventoryManagerComponent::RefillItemCharges 내부에서 걸러지므로 여기선 전 아이템을 훑기만 한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 호출하지 않는다 — 리필은 발동 순간의 사건이라 복원/조인 시 다시 채우지 않는다.
 * 인벤토리 쓰기는 서버 권위 사건이라 클라 진입은 노옵(클라는 복제로 충전량을 추종).
 * 틱하지 않으므로 비용이 없다.
 */
USTRUCT(meta = (DisplayName = "Refill Item Charges", Category = "Wx"))
struct FWxStateTreeTask_RefillItemCharges : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_RefillItemChargesInstanceData;

	FWxStateTreeTask_RefillItemCharges();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	// 대상이 로컬 플레이어 인벤토리로 고정이고 파라미터도 없어 GetDescription 으로 덧붙일 것이 없다 — 표시 이름은 DisplayName 메타가 그대로 쓰인다.
};
