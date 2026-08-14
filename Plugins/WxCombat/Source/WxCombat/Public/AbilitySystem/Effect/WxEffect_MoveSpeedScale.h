// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_MoveSpeedScale.generated.h"

/**
 * 이동 속도를 배율만큼 조절하는 무한 지속 GE.
 * 배율은 SetByCaller.MoveSpeedScale 로 실어 보낸다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_MoveSpeedScale : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_MoveSpeedScale();
};
