// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_SlowTime.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnSlowTimeFinished);

/**
 * GlobalTimeDilation을 지정한 값으로 변경한 뒤, Duration(실제 시간) 경과 시 1로 복원하는 AbilityTask.
 * 태스크가 도중에 중단되더라도 OnDestroy에서 TimeDilation을 1로 되돌린다.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_SlowTime : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_SlowTime* CreateTask(UGameplayAbility* OwningAbility, float InTimeDilation = 0.2f, float InDuration = 1.f);

	UPROPERTY()
	FWxOnSlowTimeFinished OnFinished;

	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	float TimeDilation = 0.2f;
	float Duration = 1.f;
	float StartRealTimeSeconds = 0.f;
};
