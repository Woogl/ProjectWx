// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Bleed.generated.h"

/**
 * 출혈 GameplayEffect.
 *
 * 5초 동안 0.5초 간격(총 10틱)으로 출혈 대미지를 입힌다.
 * 대미지 공식은 WxDamageExecCalc과 동일하되 가드/퍼펙트 가드/치명타를 적용하지 않는다.
 * 총 대미지를 틱 수로 균등 분할하여 적용한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Bleed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Bleed();
};
