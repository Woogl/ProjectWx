// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxMMC_Cost.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "GameplayEffect.h"

const FWxAbilityTableRow* UWxMMC_Cost::GetCostRow(const FGameplayEffectSpec& Spec) const
{
	// 컨텍스트의 소스 어빌리티 CDO는 AbilityDataRow를 그대로 가진다(EditDefaultsOnly).
	// 정적 데이터라 서버/클라 동일.
	const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.GetEffectContext().GetAbility());
	if (!Ability || Ability->AbilityDataRow.IsNull())
	{
		return nullptr;
	}
	return Ability->AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("UWxMMC_Cost::GetCostRow"));
}

float UWxMMC_MPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FWxAbilityTableRow* Row = GetCostRow(Spec);
	return Row ? -Row->MPCost : 0.f;
}

float UWxMMC_UPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FWxAbilityTableRow* Row = GetCostRow(Spec);
	return Row ? -Row->UPCost : 0.f;
}

float UWxMMC_SPCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FWxAbilityTableRow* Row = GetCostRow(Spec);
	return Row ? -Row->SPCost : 0.f;
}
