// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_GainUP.generated.h"

/**
 * Skill 피격 시 공격자 UP 회복 GameplayEffect.
 *
 * Instant 정책으로 동작하며, UP를 고정량(5) 회복한다.
 * UWxExecCalc_Damage에서 공격 적중 시 공격자에게 적용.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RecoveryUP : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UWxEffect_RecoveryUP();
};
