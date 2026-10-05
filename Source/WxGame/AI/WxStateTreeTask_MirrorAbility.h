// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "WxStateTreeTask_MirrorAbility.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
struct FWxMirrorAbilityState;


USTRUCT()
struct FWxStateTreeTask_MirrorAbilityInstanceData
{
	GENERATED_BODY()

	/** 어빌리티 AssetTags와 정확히 같은 태그가 하나라도 있으면 제외한다. 부모 태그는 매칭하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer ExcludedAbilities;

	/** 거절된 따라 쓰기를 이 시간 동안 매 틱 다시 시도한다. 플레이어 선입력 버퍼와 같은 창이다. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float RetryDuration = 0.4f;

	/**
	 * 주인의 커밋·종료 통지는 트리 틱 밖에서 오고, 그 안에서 따라 쓴 어빌리티가 트리를 멈출 수도 있다.
	 * 통지를 받는 쪽이 그때까지 살아 있도록 따라 쓰기 상태는 인스턴스 데이터 밖에 두고 공유 포인터로 든다.
	 */
	TSharedPtr<FWxMirrorAbilityState> State;
};

/**
 * 이 태스크를 둔 상태가 살아 있는 동안, 폰을 소환한 주인의 커밋과 종료를 따라 동일 클래스·레벨의 어빌리티를 실행한다.
 * 따라 쓰려고 부여한 어빌리티는 상태를 떠날 때 취소하고 걷는다.
 * 스스로 끝나지 않으므로 상태의 완료 판정에서 빠진다.
 */
USTRUCT(meta = (DisplayName = "주인 어빌리티 따라 쓰기", Category = "Wx"))
struct FWxStateTreeTask_MirrorAbility : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_MirrorAbilityInstanceData;

	FWxStateTreeTask_MirrorAbility();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
