// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_AddGP.generated.h"

class UAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class WXCOMBAT_API UWxEffect_AddGP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_AddGP();

	/** 0 이하면 no-op */
	static void ApplyTo(UAbilitySystemComponent* TargetASC, float Amount);
};
