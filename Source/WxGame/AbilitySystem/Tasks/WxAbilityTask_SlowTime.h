// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_SlowTime.generated.h"

class UWxTimeDilationSubsystem;

/**
 * 구간 동안 전역 시간 배율을 건다. 끝내는 것은 구간을 연 소유자(몽타주 이벤트 태스크)의 EndTask다.
 *
 * 배율은 서버에서만 걸고, 클라이언트에는 엔진의 WorldSettings 복제로 도착한다.
 * 도중에 중단돼도 OnDestroy에서 자기 요청만 거두고, 남은 요청 중 가장 최근 배율이 적용된다.
 */
UCLASS()
class WXGAME_API UWxAbilityTask_SlowTime : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_SlowTime* CreateTask(UGameplayAbility* OwningAbility, float InTimeDilation = 0.2f);

	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	float TimeDilation = 0.2f;

	TWeakObjectPtr<UWxTimeDilationSubsystem> TimeDilationSubsystem;
	uint64 TimeDilationHandle = 0;
};
