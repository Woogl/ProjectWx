// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Cooldown.generated.h"

/**
 * 모든 어빌리티가 함께 쓰는 쿨다운 GE. UWxAbilityBase의 CooldownGameplayEffectClass 기본값이다.
 * 쿨다운의 식별자는 어빌리티의 CooldownTags이고, 어빌리티가 적용할 때 스펙에 붙인다. 같은 태그를 고른 어빌리티끼리 쿨다운을 나눠 쓴다.
 *
 * 엔진은 스택을 GE 클래스 단위로 합치므로 쌓지 않는다. 소모한 충전 하나가 이 GE 하나다.
 * 충전 상한은 GE가 아니라 어빌리티의 MaxRecharges이며 UWxAbilityBase::CheckCooldown이 판정한다.
 * 지속시간은 SetByCaller.Duration이며 UWxAbilityBase::ApplyCooldown이 채운다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Cooldown();
};
