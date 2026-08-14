// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_DrainSP.generated.h"

/**
 * 질주 SP 소모 GameplayEffect.
 * Movement.Sprint를 보유한 동안에만 틱하므로, 어빌리티는 활성화 시 한 번 걸어 두고 태그 토글로 소모를 켜고 끈다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_DrainSP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_DrainSP();

	/** MaxSP에서 0까지 소진되는 데 걸리는 시간(초) */
	static constexpr float FullDrainDuration = 8.0f;

	/** 소모 틱 간격(초) */
	static constexpr float DrainPeriod = 1.0f / 30.0f;
};
