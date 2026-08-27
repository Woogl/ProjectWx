// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_AddDP.generated.h"

class UAbilitySystemComponent;

/**
 * 어트리뷰트에 직접 쓰면 PreAttributeBaseChange의 MaxDP 클램프를 지나지 않아 base가 상한을 넘고, 그로기 드레인이 DP를 0까지 내리지 못한다.
 * 대미지의 DP 가산과 같은 경로를 타도록 GE로 건다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_AddDP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_AddDP();

	/** 0 이하면 no-op */
	static void ApplyTo(UAbilitySystemComponent* TargetASC, float Amount);
};
