// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "WxStateTreeTask_MirrorMovement.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AAIController;
class ACharacter;
class UAbilitySystemComponent;


USTRUCT()
struct FWxStateTreeTask_MirrorMovementInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector LocalOffset = FVector(0.f, 50.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1.0"))
	float ArrivalRadius = 15.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.01"))
	float TeleportDelay = 1.f;

	/** 일치하는 AssetTags의 발동 시 전방으로 이동한다. 같은 소유 태그가 유지되는 동안 추적 대신 주인을 바라본다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTagContainer FaceMasterAbilityTags;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1.0", Units = "cm"))
	float AbilityTeleportDistance = 500.f;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> Master;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> Follower;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> FollowerAbilitySystem;

	UPROPERTY()
	FActiveGameplayEffectHandle MoveSpeedEffectHandle;

	FDelegateHandle AbilityActivatedHandle;

	/** 지난 틱에 살아 있던 몽타주 인스턴스. 여기서 사라진 것이 있으면 그 사이에 몽타주가 끝난 것이다. */
	TArray<int32> MontageInstanceIDs;

	bool bPendingMontageEndTeleport = false;

	float TravelTime = 0.f;
};

/**
 * 이 태스크를 둔 상태가 살아 있는 동안 폰을 소환한 주인 옆으로 이동시키며, 몽타주 종료 또는 도달 제한 시간 초과 시 위치를 보정한다.
 * 스스로 끝나지 않으므로 상태의 완료 판정에서 빠진다.
 */
USTRUCT(meta = (DisplayName = "주인 이동 따라가기", Category = "Wx"))
struct FWxStateTreeTask_MirrorMovement : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_MirrorMovementInstanceData;

	FWxStateTreeTask_MirrorMovement();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	/** 걸어 둔 구독·감속·틱 순서를 되돌린다. 컨트롤러가 없어도 폰 쪽은 되돌려야 하므로 선택 인자로 받는다. */
	void Release(AAIController* Controller, FInstanceDataType& Instance) const;
};
