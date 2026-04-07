// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Cooldown.h"

UWxEffect_Cooldown::UWxEffect_Cooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));

	// 어빌리티의 충전(Charge) 시스템을 GE 스택으로 표현한다.
	//   - 1 stack = 소모된 충전 1회
	//   - ApplyCooldown 시 스택 1 추가 (GAS가 알아서 누적)
	//   - 스택 만료 시 RemoveSingleStackAndRefreshDuration로 1 stack 제거 후 다음 재충전 타이머 자동 시작
	// MaxCharges 한도 검사는 어빌리티 측 CheckCooldown에서 수행한다.
	// UE 5.7: StackingType은 deprecation 경고가 있지만 런타임 setter는 WITH_EDITOR 전용이므로
	// CDO 생성자에서는 직접 대입이 유일한 방법이다. UE 본체도 동일 패턴으로 설정한다.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0; // 무제한 (어빌리티가 직접 제한)
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}
