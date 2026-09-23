// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_IgnoreCooldowns.generated.h"

/**
 * 유지되는 동안 쿨다운 GE(Cooldown 태그를 부여하는 GE)의 적용을 막고, 적용 시점에 걸려 있던 쿨다운도 걷어낸다.
 * 주인의 어빌리티를 따라 쓰는 소환물 등의 AbilitySet이 부여한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_IgnoreCooldowns : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_IgnoreCooldowns();
};
