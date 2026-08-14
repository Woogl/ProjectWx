// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Invincible.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * 무적 상태(Effect.Invincible)를 부여한다.
 *
 * 정의는 지속시간이 없어, 어빌리티가 ActivationOwnedEffects로 걸면 그 어빌리티가 수명을 쥔다.
 * 구간 길이가 정해진 무적(노티파이·컷신)은 ApplyTo로 그 길이를 스펙에 실어 스스로 만료되게 한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_Invincible : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Invincible();

	/** PredictingAbility의 활성화 예측 키로 적용해 소유 클라이언트도 같은 프레임에 태그를 갖는다. 널이면 서버에서만 걸리고 클라는 복제로 받는다. */
	static FActiveGameplayEffectHandle ApplyTo(UAbilitySystemComponent* TargetASC, float Duration, const UGameplayAbility* PredictingAbility);
};
