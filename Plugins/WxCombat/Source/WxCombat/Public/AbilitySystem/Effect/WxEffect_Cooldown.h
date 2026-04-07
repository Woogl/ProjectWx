// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * 범용 쿨다운 GameplayEffect.
 *
 * Duration 정책으로 동작하며, 지속 시간은 UWxAbility::ApplyCooldown에서 동적으로 설정한다.
 * 충전(Charge) 시스템을 GE 스택으로 표현한다 — 스택 1개 = 소모된 충전 1회.
 * MaxCharges가 1이면 단순 단일 쿨다운으로, 2 이상이면 충전 시스템으로 동작한다.
 *
 * UWxAbility::ApplyCooldown에서 자동으로 사용되므로 직접 인스턴스화할 필요 없음.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
