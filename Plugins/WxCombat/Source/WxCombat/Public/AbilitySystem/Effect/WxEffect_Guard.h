// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Guard.generated.h"

/**
 * 방어 유효 상태(Effect.Guard)를 부여한다.
 * 가드는 키를 쥔 동안 유지돼 고정 지속시간이 없으므로, 수명은 가드 어빌리티가 끊는다 — 가드 종료와 SP 고갈로 가드가 깨질 때.
 *
 * 방어 판정의 감소율은 ExecCalc가 Effect.Guard 태그를 본 뒤 곱한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Guard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Guard();

	/** 가드 중 받는 대미지 배율. 0.5면 50% 감소. 저작 대상이 아니라 코드 고정값이다. */
	static constexpr float DamageMultiplier = 0.5f;
};
