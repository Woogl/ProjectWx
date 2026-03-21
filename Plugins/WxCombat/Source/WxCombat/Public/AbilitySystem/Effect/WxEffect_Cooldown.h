// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * 범용 쿨다운 GameplayEffect.
 *
 * Duration 정책으로 동작하며, 지속 시간은 UWxAbility::ApplyCooldown에서 동적으로 설정한다.
 * UWxAbility::ApplyCooldown에서 자동으로 사용되므로 직접 인스턴스화할 필요 없음.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
