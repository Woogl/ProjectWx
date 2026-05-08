// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_RecoverResource.generated.h"

class UAbilitySystemComponent;

/**
 * 공격자 자원 회복 GameplayEffect.
 *
 * Instant 정책으로 UP/MP를 한 번에 회복한다.
 * 일반적으로 ApplyTo 정적 진입점을 통해 적용한다. (ExecCalc/어빌리티 공용)
 */
UCLASS()
class WXCOMBAT_API UWxEffect_RecoverResource : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_RecoverResource();

	/** UP/MP 회복 GE를 ASC 자신에게 즉시 적용. 두 값 모두 0 이하면 no-op. */
	static void ApplyTo(UAbilitySystemComponent* TargetASC, float UP, float MP);
};
