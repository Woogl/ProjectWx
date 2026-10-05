// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "GameplayAbilitySpecHandle.h"
#include "WxAbilityTask_Rush.generated.h"

class UAnimInstance;
class UWxAbilityTask_LockMovementRotation;
struct FAbilityEndedData;

/** 교차 돌진의 출발점과 충돌 원복을 이동 태스크 수명에 묶는다. */
UCLASS()
class WXGAME_API UWxAbilityTask_Rush : public UAbilityTask_ApplyRootMotionMoveToForce
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_Rush* CreateTask(UGameplayAbility* OwningAbility, int32 MontageInstanceID, AActor& Other, float InDuration, float StopDistance, const TArray<TEnumAsByte<EObjectTypeQuery>>& InIgnoreCollisions);
	static UWxAbilityTask_Rush* FindRush(AActor& Actor);

	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool AbilityIsEnding) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void SharedInitAndApply() override;

private:
	void ReleaseState();
	void TrackPartner();
	void HandleTargetAbilityEnded(const FAbilityEndedData& Data);

	UFUNCTION()
	void HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UPROPERTY(Replicated)
	TArray<TEnumAsByte<EObjectTypeQuery>> IgnoreCollisions;

	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<UAnimInstance> SourceAnimInstance;
	TWeakObjectPtr<UWxAbilityTask_LockMovementRotation> RotationLock;
	int32 SourceMontageInstanceID = INDEX_NONE;
	FGameplayAbilitySpecHandle AbilityHandle;
	FGameplayAbilitySpecHandle PartnerAbilityHandle;
	FCollisionResponseContainer SavedCollisionResponses;
	bool bOwnsState = false;
	bool bChangedCollisionResponses = false;
};
