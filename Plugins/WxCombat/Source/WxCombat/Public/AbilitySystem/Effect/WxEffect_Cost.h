// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cost.generated.h"

/**
 * 어빌리티 공용 코스트 GameplayEffect.
 *
 * Instant 정책으로 동작하며, Modifiers는 UWxAbilityBase::GetCostGameplayEffect()가 호출 시점의 MPCost/UPCost 값으로 채운다.
 * 엔진 순정 CheckCost(CanApplyAttributeModifiers)/ApplyCost가 이 GE를 그대로 소비하므로 직접 인스턴스화할 필요 없음.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cost();
};
