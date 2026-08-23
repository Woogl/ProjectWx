// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_PerfectGuard.generated.h"

/**
 * 퍼펙트 가드 판정 구간(Effect.PerfectGuard)을 부여한다.
 * 구간 길이가 애니메이션마다 달라 정의에 두지 않고 UWxCombatLibrary::ApplyEffectForDuration이 스펙에서 잠근다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_PerfectGuard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_PerfectGuard();
};
