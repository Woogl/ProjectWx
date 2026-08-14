// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_PerfectGuard.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * 퍼펙트 가드 판정 구간(Effect.PerfectGuard)을 부여한다.
 * 구간 길이가 애니메이션마다 달라 정의에 두지 않고 스펙에서 잠근다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_PerfectGuard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_PerfectGuard();

	/** PredictingAbility의 활성화 예측 키로 적용해 소유 클라이언트도 같은 프레임에 태그를 갖는다. 널이면 서버에서만 걸리고 클라는 복제로 받는다. */
	static FActiveGameplayEffectHandle ApplyTo(UAbilitySystemComponent* TargetASC, float Duration, const UGameplayAbility* PredictingAbility);
};
