// Copyright Woogle. All Rights Reserved.

#pragma once

#include "RootMotionModifier_SkewWarp.h"
#include "GameplayAbilitySpecHandle.h"
#include "Engine/EngineTypes.h"
#include "WxRootMotionModifier_Rush.generated.h"

class UBrainComponent;
class UAbilitySystemComponent;
class UAnimNotifyState;
struct FAbilityEndedData;

/** 에셋인 NotifyState 대신 캐릭터별 modifier가 돌진 상태와 원복 책임을 소유한다. */
UCLASS()
class WXCOMBAT_API UWxRootMotionModifier_Rush : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	bool InitializeRush(AActor& Other, const UAnimNotifyState& Notify, float StopDistance, const TArray<TEnumAsByte<EObjectTypeQuery>>& IgnoreCollisions);
	bool MatchesNotify(const UAnimNotifyState& Notify) const;
	void CancelRush();
	virtual void OnStateChanged(ERootMotionModifierState LastState) override;
	virtual void Update(const FMotionWarpingUpdateContext& Context) override;

private:
	void ReleaseState();
	UFUNCTION()
	void HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	void HandleTargetAbilityEnded(const FAbilityEndedData& Data);

	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<const UAnimNotifyState> SourceNotify;
	TWeakObjectPtr<UBrainComponent> PausedBrain;
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;
	FGameplayAbilitySpecHandle AbilityHandle;
	FVector SourceLocation = FVector::ZeroVector;
	bool bOwnsState = false;
	FCollisionResponseContainer SavedCollisionResponses;
	bool bChangedCollisionResponses = false;
	bool bSavedControllerYaw = false;
	bool bSavedPhysicsRotation = false;
};
