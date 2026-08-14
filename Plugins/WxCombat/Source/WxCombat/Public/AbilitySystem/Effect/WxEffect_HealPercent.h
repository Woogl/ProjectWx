// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_HealPercent.generated.h"

/**
 * HP를 MaxHP의 40%만큼 즉시 회복시키는 GameplayEffect.
 * HP는 어트리뷰트 세트에서 [0, MaxHP]로 클램프되므로 오버힐은 발생하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_HealPercent : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_HealPercent();
};
