// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_MPRecovery.generated.h"

/**
 * 피격 시 공격자 MP 회복 GameplayEffect.
 *
 * Instant 정책으로 동작하며, MP를 고정량(5) 회복한다.
 * UWxDamageExecCalc에서 공격 적중 시 공격자에게 적용.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_MPRecovery : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_MPRecovery();
};
