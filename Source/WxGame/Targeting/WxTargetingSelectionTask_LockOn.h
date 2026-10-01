// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "WxTargetingSelectionTask_LockOn.generated.h"

/**
 * 결과에 이미 있는 락온 대상을 선두로 옮긴다. 락온이 없거나 대상이 결과에 없으면 기존 결과를 유지한다.
 * AOE 및 일반 정렬 뒤에 배치해야 최종 락온 우선순위가 유지된다.
 */
UCLASS()
class WXGAME_API UWxTargetingSelectionTask_LockOn : public UTargetingTask
{

	GENERATED_BODY()

public:
	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};
