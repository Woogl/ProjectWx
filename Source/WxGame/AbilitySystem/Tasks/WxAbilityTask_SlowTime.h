// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_SlowTime.generated.h"

class UWxTimeDilationSubsystem;

/**
 * 시간은 딜레이션이 반영된 게임 시계로 잰다 — 애니메이션도 같은 시계로 흐르므로, 구간을 연 몽타주 노티파이의 길이를 그대로 받으면 둘이 함께 끝난다.
 *
 * 배율은 서버에서만 걸고, 클라이언트에는 엔진의 WorldSettings 복제로 도착한다.
 * 도중에 중단돼도 OnDestroy에서 자기 요청만 거두고, 남은 요청 중 가장 최근 배율이 적용된다.
 */
UCLASS()
class WXGAME_API UWxAbilityTask_SlowTime : public UAbilityTask
{
	GENERATED_BODY()

public:
	/** InDuration < 0이면 소유자가 몽타주 구간 종료 시 EndTask로 정리한다. */
	static UWxAbilityTask_SlowTime* CreateTask(UGameplayAbility* OwningAbility, float InTimeDilation = 0.2f, float InDuration = 1.f);

	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	float TimeDilation = 0.2f;
	float Duration = 1.f;

	TWeakObjectPtr<UWxTimeDilationSubsystem> TimeDilationSubsystem;
	uint64 TimeDilationHandle = 0;
};
