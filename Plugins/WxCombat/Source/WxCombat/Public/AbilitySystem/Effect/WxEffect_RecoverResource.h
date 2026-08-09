// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_RecoverResource.generated.h"

class UAbilitySystemComponent;

/**
 * 공격자 자원 회복 GameplayEffect.
 * ExecCalc와 어빌리티가 ApplyTo 진입점을 공유한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RecoverResource : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_RecoverResource();

	/** 두 값 모두 0 이하면 no-op */
	static void ApplyTo(UAbilitySystemComponent* TargetASC, float UP, float MP);
};
