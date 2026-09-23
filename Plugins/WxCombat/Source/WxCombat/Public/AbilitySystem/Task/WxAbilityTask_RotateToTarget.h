// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_RotateToTarget.generated.h"

class UWxLockOnComponent;

/** 아바타 락온 컴포넌트의 대상을 매 틱 읽어 캐릭터의 요(yaw)를 그쪽으로 보간한다. */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_RotateToTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_RotateToTarget* CreateTask(UGameplayAbility* OwningAbility, float InInterpSpeed = 10.f);

	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void Activate() override;

private:
	TWeakObjectPtr<UWxLockOnComponent> LockOnComponent;
	float InterpSpeed = 10.f;
};
