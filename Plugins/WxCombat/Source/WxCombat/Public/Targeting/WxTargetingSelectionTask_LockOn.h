// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "WxTargetingSelectionTask_LockOn.generated.h"

/**
 * 락온 대상을 결과에 추가하고 선두에 배치한다. 락온이 없으면 기존 결과를 유지한다.
 * AOE 및 일반 정렬 뒤에 배치해야 최종 락온 우선순위가 유지된다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingSelectionTask_LockOn : public UTargetingTask
{

	GENERATED_BODY()

public:
	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};
