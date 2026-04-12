// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * 공용 쿨다운 GameplayEffect.
 *
 * 모든 어빌리티가 공유하는 단일 GE 클래스. 스태킹을 사용하지 않으며,
 * 어빌리티 사용 시마다 개별 인스턴스가 생성된다.
 * Duration은 어빌리티의 ApplyCooldown에서 FGameplayEffectSpec::SetDuration으로 직접 설정된다.
 * 소스 어빌리티 CDO로 개별 어빌리티의 쿨다운을 구분한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
