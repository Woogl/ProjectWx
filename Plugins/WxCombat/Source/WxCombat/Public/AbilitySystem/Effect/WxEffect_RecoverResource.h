// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_RecoverResource.generated.h"

/**
 * 공격자 자원 회복 GameplayEffect.
 *
 * Instant 정책으로 UP/MP를 한 번에 회복한다.
 * 호출 측(WxExecCalc_Damage, Dodge/Guard 등)은 Spec에
 * SetByCaller.Recovery.UP / SetByCaller.Recovery.MP 값을 모두 세팅해야 한다.
 * (미설정 시 FSetByCallerFloat 경고가 발생하며, 해당 모디파이어는 0으로 평가된다.)
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RecoverResource : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_RecoverResource();
};
