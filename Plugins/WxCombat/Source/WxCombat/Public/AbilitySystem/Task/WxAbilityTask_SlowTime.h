// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_SlowTime.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnSlowTimeFinished);

/**
 * GlobalTimeDilation을 지정한 값으로 변경한 뒤, Duration(실제 시간) 경과 시 되돌리는 AbilityTask.
 * 태스크가 도중에 중단되더라도 OnDestroy에서 자기가 건 딜레이션을 거둔다.
 *
 * 딜레이션은 UWxTimeDilationComponent가 서버 권위로 관리한다.
 * 그 사이 다른 요청이 배율을 가져갔다면 이 태스크의 해제는 무시되어 남의 연출을 끊지 않는다.
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
