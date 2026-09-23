// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_MoveSpeedOverride.generated.h"

/**
 * 무한 지속 GE. SPD 를 SetByCaller.MoveSpeedScale 값으로 덮어써 대상에 걸린 다른 속도 효과를 무시한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_MoveSpeedOverride : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_MoveSpeedOverride();
};
