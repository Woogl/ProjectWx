// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "WxAnimNotifyState_Rush.generated.h"

class APawn;

UENUM(BlueprintType)
enum class EWxRushTarget : uint8
{
	LockOnTarget,
	Master,
	Minion,
};

/** 구간 시작에 목적지를 고정하고 루트 모션을 보정한다. 피해는 WeaponAttack 구간으로 별도 설정한다. */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_Rush : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UWxAnimNotifyState_Rush();
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Rush")
	EWxRushTarget TargetSource = EWxRushTarget::LockOnTarget;

	/** 대상 앞에서 멈출 거리. 교차 돌진에는 0을 사용한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Rush", meta = (ClampMin = "0"))
	float StopDistance = 0.f;

	/** 돌진 구간 동안 캡슐이 무시할 오브젝트 종류. 빈 목록은 기존 충돌을 유지한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Rush")
	TArray<TEnumAsByte<EObjectTypeQuery>> IgnoreCollisions;

	/** 소환물을 대상으로 돌진할 때 같은 구간에서 협공을 시작한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Rush", meta = (EditCondition = "TargetSource == EWxRushTarget::Minion"))
	bool bCommandMinion = false;

	UPROPERTY(EditAnywhere, Category = "Wx|Rush", meta = (Categories = "Ability", EditCondition = "bCommandMinion"))
	FGameplayTag MinionAbilityTag;

private:
	AActor* FindTarget(APawn& Avatar) const;
};
