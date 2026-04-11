// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_BuffATK.generated.h"

/**
 * ATK 증가 GameplayEffect.
 *
 * Duration 정책으로 동작하며, 5초 동안 ATK를 50% 증가시킨다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_BuffATK : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_BuffATK();
};
