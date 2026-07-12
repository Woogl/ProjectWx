// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_NoCooldown.generated.h"

class UAbilitySystemComponent;

/**
 * 지속시간 동안 쿨다운 적용을 차단하는 GameplayEffect.
 *
 * 적용 시 RemoveOtherGameplayEffectComponent로 기존 쿨다운을 모두 제거하고, Immunity 컴포넌트를 사용하여 이후 UWxEffect_Cooldown의 적용을 차단한다.
 * 지속시간은 SetByCaller.Duration 태그로 지정한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_NoCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_NoCooldown();

	/** 대상 ASC에 기존 쿨다운을 모두 제거하고 NoCooldown을 적용한다 */
	static FActiveGameplayEffectHandle ApplyToASC(UAbilitySystemComponent* ASC, float Duration);
};
