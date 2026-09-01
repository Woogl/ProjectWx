// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_NoCooldown.generated.h"

/**
 * 지속시간 동안 쿨다운(Cooldown 태그를 부여하는 GE)의 적용을 차단하는 GameplayEffect.
 * 적용 시점에 걸려 있던 쿨다운도 모두 제거한다.
 * 지속시간은 SetByCaller.Duration 태그로 지정한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_NoCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_NoCooldown();
};
