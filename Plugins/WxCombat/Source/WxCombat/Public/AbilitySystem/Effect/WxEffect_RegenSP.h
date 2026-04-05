// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_RegenSP.generated.h"

/**
 * SP 자연 회복 GameplayEffect.
 *
 * Infinite 정책으로 1/30초 간격으로 SP를 회복한다.
 * 틱당 회복량은 MaxSP / 120 으로, SP가 0인 상태에서 4초 후 MaxSP에 도달한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RegenSP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_RegenSP();

	/** SP 0에서 MaxSP까지 회복에 걸리는 시간(초) */
	static constexpr float FullRegenDuration = 4.0f;

	/** 회복 틱 간격(초) : 1/30초 */
	static constexpr float RegenPeriod = 1.0f / 30.0f;
};
