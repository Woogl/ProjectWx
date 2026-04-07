// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));

	// 충전(Charge) 시스템을 GE 스택으로 표현한다.
	//   - 스택 1개 = 소모된 충전 1회
	//   - 스택 만료 시 RemoveSingleStackAndRefreshDuration: 스택 1개 제거 후 Duration 리셋
	//     → 충전이 D초 간격으로 순차 복구된다 (D, 2D, 3D, ...)
	//   - NeverRefresh: 추가 스택이 쌓여도 진행 중인 Duration을 리셋하지 않음
	// MaxCharges 한도 검사는 CheckCooldown에서 수행한다.
	// UE 5.7: StackingType은 deprecation 경고가 있지만 CDO 생성자에서는 직접 대입이 유일한 방법이다.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}
