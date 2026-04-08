// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_CooldownBase.generated.h"

/**
 * 쿨다운 GameplayEffect 베이스 클래스.
 *
 * 이 클래스는 GE 스태킹 정책만 정의한다. 어빌리티마다 이 클래스를 상속한 BP 에셋을 만들고
 *   - Duration Magnitude (쿨다운 1주기 시간)
 *   - Stack Limit Count (최대 충전 수, 1이면 단일 쿨다운)
 *   - Granted Tags (해당 어빌리티의 Cooldown_* 태그)
 * 를 BP에서 설정한다. 어빌리티 BP의 CooldownGameplayEffectClass 슬롯에 BP 에셋을 지정하면 끝.
 *
 * 충전(Charge) 시스템 동작 원리:
 *   StackExpirationPolicy = RemoveSingleStackAndRefreshDuration
 *   StackDurationRefreshPolicy = NeverRefresh
 *   → 어빌리티 사용 시 스택 +1, Duration 경과마다 스택 -1씩 순차 복구.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_CooldownBase : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_CooldownBase();
};
