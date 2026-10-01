// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_LockMovementRotation.generated.h"

class UCharacterMovementComponent;

/** 태스크 수명 동안 CMC의 자동 회전 속도를 잠근다. 중첩된 마지막 잠금이 끝날 때 원복한다. */
UCLASS()
class WXGAME_API UWxAbilityTask_LockMovementRotation : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_LockMovementRotation* CreateTask(UGameplayAbility* OwningAbility);
	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

private:
	UWxAbilityTask_LockMovementRotation* FindOtherLock() const;
	TWeakObjectPtr<UCharacterMovementComponent> LockedMovement;
	FRotator SavedRotationRate = FRotator::ZeroRotator;
};
