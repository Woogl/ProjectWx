// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_RegenPP.generated.h"

/**
 * PP 자연 회복 GameplayEffect.
 *
 * Infinite 정책으로 1/30초 간격으로 PP를 회복한다.
 * 틱당 회복량은 MaxPP / 120 으로, PP가 0인 상태에서 4초 후 MaxPP에 도달한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RegenPP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_RegenPP();

	/** PP 0에서 MaxPP까지 회복에 걸리는 시간(초) */
	static constexpr float FullRegenDuration = 4.0f;

	/** 회복 틱 간격(초) : 1/30초 */
	static constexpr float RegenPeriod = 1.0f / 30.0f;
};
