// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Reflect.generated.h"

/**
 * 퍼펙트 가드 DP 반사 GameplayEffect.
 *
 * Instant 정책으로 동작하며, DP를 SetByCaller(SetByCaller.ReflectDP) 만큼 가산한다.
 * UWxExecCalc_Damage에서 퍼펙트 가드 성공 시 공격자에게 적용.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Reflect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Reflect();
};
