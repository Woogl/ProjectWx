// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Damage.generated.h"

/**
 * 대미지 GameplayEffect.
 *
 * Instant 정책으로 동작하며, WxExecCalc_Damage을 통해 대미지를 계산한다.
 * SetByCaller.FixedDamage 가 양수이면 고정 대미지 모드로 분기하여 ATK/DEF/Crit/가드/HitReact 등
 * 전투 파이프라인을 우회하고 환경 대미지(낙사·장판 등)로 처리한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Damage();
};
