// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Invincible.generated.h"

/**
 * 무적 상태(Effect.Invincible)를 부여한다.
 *
 * 정의에 지속시간이 없어 수명은 언제나 거는 쪽이 쥔다.
 * 어빌리티는 ActivationOwnedEffects로, 판정 구간은 애님 노티파이가, 컷신은 재생 태스크가 각각 시작에 걸고 끝에서 걷어낸다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Invincible : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Invincible();
};
