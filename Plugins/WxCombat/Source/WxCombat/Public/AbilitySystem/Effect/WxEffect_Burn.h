// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Burn.generated.h"

/**
 * 지속시간·주기 상수와 틱 대미지 공식은 UWxExecCalc_Burn이 소유한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_Burn : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Burn();
};
