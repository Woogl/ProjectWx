// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxMMC_CooldownDuration.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

float UWxMMC_CooldownDuration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly). 정적 데이터라 서버/클라 동일.
	const UGameplayAbility* SourceAbility = Spec.GetEffectContext().GetAbility();
	const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(SourceAbility);
	if (!WxAbility || WxAbility->AbilityDataRow.IsNull())
	{
		return 0.f;
	}

	const FWxAbilityTableRow* Row = WxAbility->AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("UWxMMC_CooldownDuration"));
	if (!Row || Row->CooldownTime <= 0.f)
	{
		return 0.f;
	}

	// 직렬 회복 — 기존 쿨다운 중 가장 늦게 만료되는 잔여시간 뒤에 이번 CooldownTime을 이어붙인다.
	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	if (UAbilitySystemComponent* ASC = Spec.GetEffectContext().GetInstigatorAbilitySystemComponent())
	{
		WxAbility->QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration);
	}

	return LongestRemaining + Row->CooldownTime;
}
