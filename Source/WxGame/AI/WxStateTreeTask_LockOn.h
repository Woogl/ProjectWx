// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "WxStateTreeTask_LockOn.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AActor;
class AAIController;
class APawn;


USTRUCT()
struct FWxStateTreeTask_LockOnInstanceData
{
	GENERATED_BODY()

	/** 포커스와 회전 모드를 걸어 둔 폰. 비어 있으면(IsExplicitlyNull) 적용 기록이 없다는 뜻이다. */
	UPROPERTY()
	TWeakObjectPtr<APawn> LockedOnPawn;
};

/**
 * 이 태스크를 둔 상태가 살아 있는 동안 AWxAIController 가 겨누는 대상을 컨트롤러 포커스와 폰의 strafe 회전 모드에 반영한다.
 * AI 판 락온이며, 플레이어의 락온과는 별개의 구현이다 — 겨누는 대상 자체는 컨트롤러가 UWxLockOnComponent 에 실어 두고, 이 태스크는 그 대상을 어떻게 바라볼지만 정한다.
 *
 * 컨트롤러의 포커스와 폰의 회전 모드는 이 태스크가 한 쌍으로 단독 소유한다.
 * Gameplay 우선순위 포커스도 이 태스크만 쓴다는 전제다.
 *
 * 스스로 끝나지 않으므로 상태의 완료 판정에서 빠진다. 이 태스크만 둔 상태에는 판정에 참여하는 태스크를 따로 둔다.
 */
USTRUCT(meta = (DisplayName = "Lock On", Category = "Wx"))
struct FWxStateTreeTask_LockOn : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_LockOnInstanceData;

	FWxStateTreeTask_LockOn();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	void SyncLockOn(FStateTreeExecutionContext& Context) const;

	void ApplyLockOn(AAIController& AIController, APawn& Pawn, AActor& Target, FInstanceDataType& Instance) const;

	/**
	 * 적용 기록이 있을 때만 되돌린다. 멱등이라 틱과 상태 이탈 양쪽에서 불러도 안전하다.
	 * 컨트롤러가 없어도 폰의 회전 모드는 되돌려야 하므로 컨트롤러를 선택 인자로 받는다.
	 */
	void ReleaseLockOn(AAIController* AIController, FInstanceDataType& Instance) const;
};
