// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Hit.generated.h"

/** 스탯 변경 없이 타격 판정과 후속 피해·반응을 조정한다. */
UCLASS()
class WXCOMBAT_API UWxEffect_Hit : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Hit();
};
