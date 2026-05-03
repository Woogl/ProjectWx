// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_FixedDamage.generated.h"

/**
 * 고정 대미지 GameplayEffect.
 *
 * Instant 정책으로 IncomingDamage 메타 어트리뷰트에 SetByCaller.FixedDamage 값을 더한다.
 * ATK/DEF/Crit/가드/PG/HitReact/GameplayCue 등 전투 파이프라인을 모두 우회하므로
 * 낙사·장판·가시·스크립트성 디버프 등 환경 대미지 용도로 사용한다.
 *
 * IncomingDamage 경로를 사용하므로 HP 차감과 사망 처리(State_Dead 부여)는 정상 동작.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_FixedDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_FixedDamage();
};
