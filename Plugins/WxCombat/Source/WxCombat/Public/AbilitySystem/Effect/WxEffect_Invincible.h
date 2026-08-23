// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Invincible.generated.h"

/**
 * 무적 상태(Effect.Invincible)를 부여한다.
 *
 * 정의는 지속시간이 없어, 어빌리티가 ActivationOwnedEffects로 걸면 그 어빌리티가 수명을 쥔다.
 * 구간 길이가 정해진 무적(노티파이·컷신)은 UWxCombatLibrary::ApplyEffectForDuration으로 그 길이를 스펙에 실어 스스로 만료되게 한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Invincible : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Invincible();
};
