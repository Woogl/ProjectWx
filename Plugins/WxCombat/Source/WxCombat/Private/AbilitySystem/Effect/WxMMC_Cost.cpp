// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxMMC_Cost.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "GameplayEffect.h"

float UWxMMC_Cost::GetCostMagnitude(const FGameplayEffectSpec& Spec, EWxAbilityCostResource Resource) const
{
	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly).
	// 정적 데이터라 서버/클라 동일.
	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetEffectContext().GetAbility());
	if (!Ability || Ability->AbilityDataRow.IsNull())
	{
		return 0.f;
	}

	const FWxAbilityTableRow* Row = Ability->AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("UWxMMC_Cost::GetCostMagnitude"));
	return Row && Row->CostResource == Resource ? -Row->CostAmount : 0.f;
}

float UWxMMC_MPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return GetCostMagnitude(Spec, EWxAbilityCostResource::MP);
}

float UWxMMC_UPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return GetCostMagnitude(Spec, EWxAbilityCostResource::UP);
}

float UWxMMC_SPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	return GetCostMagnitude(Spec, EWxAbilityCostResource::SP);
}
