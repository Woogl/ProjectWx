// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "WxEffectComp_SendHitReactEvent.generated.h"

/**
 * GE 적용 시 대상에게 Event.HitReact 이벤트를 발송하는 GameplayEffectComponent.
 *
 * 대미지 GE에 이 컴포넌트를 추가하면 GE 적용 시점에
 * 대상 액터에게 Event.HitReact GameplayEvent가 발송되어
 * GA_HitReact 어빌리티가 트리거된다.
 */
UCLASS(DisplayName = "Send HitReact Event")
class WXCOMBAT_API UWxEffectComp_SendHitReactEvent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
