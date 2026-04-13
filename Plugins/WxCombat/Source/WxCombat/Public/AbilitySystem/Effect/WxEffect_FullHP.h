// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_FullHP.generated.h"

/**
 * HP를 MaxHP로 즉시 회복시키는 GameplayEffect.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_FullHP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_FullHP();
};
