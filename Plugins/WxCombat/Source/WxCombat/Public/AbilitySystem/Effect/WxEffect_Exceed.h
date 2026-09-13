// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Exceed.generated.h"

/**
 * 일정 시간 ATK와 ASPD를 증가시킨다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_Exceed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Exceed();
};
