// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_RegenSP.generated.h"

/**
 * 가드 중, 질주 중, 탈진 중에는 회복이 억제된다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RegenSP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_RegenSP();

	/** SP 0에서 MaxSP까지 회복에 걸리는 시간(초) */
	static constexpr float FullRegenDuration = 4.0f;

	static constexpr float RegenPeriod = 1.0f / 30.0f;
};
