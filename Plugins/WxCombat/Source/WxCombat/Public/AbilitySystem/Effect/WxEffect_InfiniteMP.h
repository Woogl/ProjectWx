// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_InfiniteMP.generated.h"

/**
 * 지속시간 동안 MP를 항상 최대치로 유지하는 GameplayEffect.
 *
 * 지속시간은 SetByCaller.Duration 태그로 지정한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_InfiniteMP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_InfiniteMP();

	static constexpr float TickPeriod = 1.0f / 30.0f;
};
