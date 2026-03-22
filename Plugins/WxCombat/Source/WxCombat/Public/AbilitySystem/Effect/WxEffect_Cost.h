// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cost.generated.h"

/**
 * 범용 코스트 GameplayEffect.
 *
 * Instant 정책으로 동작하며, MP를 SetByCaller(SetByCaller.Cost.MP) 만큼 차감한다.
 * UWxAbility::ApplyCost에서 자동으로 사용되므로 직접 인스턴스화할 필요 없음.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cost();
};
