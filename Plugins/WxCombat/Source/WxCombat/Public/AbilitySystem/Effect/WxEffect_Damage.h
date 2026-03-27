// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Damage.generated.h"

/**
 * 대미지 GameplayEffect.
 *
 * Instant 정책으로 동작하며, WxDamageExecCalc을 통해 대미지를 계산한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Damage();
};
