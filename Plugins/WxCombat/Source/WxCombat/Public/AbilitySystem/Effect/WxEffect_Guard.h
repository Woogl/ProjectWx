// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Guard.generated.h"

/**
 * 방어 유효 상태(Effect.Guard)를 부여한다. 가드는 키를 쥔 동안 유지돼 고정 지속시간이 없으므로,
 * 수명은 가드 어빌리티가 끊는다 — 가드 종료와 SP 고갈로 가드가 깨질 때.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Guard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Guard();
};
