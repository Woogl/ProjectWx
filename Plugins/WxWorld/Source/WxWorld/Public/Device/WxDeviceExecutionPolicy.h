// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

/** 상태 적용은 복구에서도 실행하고, 일회성 효과는 실제 진입에서만 실행한다. */
struct FWxDeviceExecutionPolicy
{
	static bool IsRestoring(const FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition);
	static bool IsRestoringDevice(const FStateTreeExecutionContext& Context);
};
