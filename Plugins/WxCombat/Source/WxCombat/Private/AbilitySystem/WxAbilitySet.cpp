// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySet.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Attribute/WxCombatAttributeInitTableRow.h"

void UWxAbilitySet::GiveToAbilitySystem(UWxAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return;
	}
	
	if (const FWxCombatAttributeInitTableRow* Row = AttributeInitRow.GetRow<FWxCombatAttributeInitTableRow>(TEXT("WxAbilitySet::GiveToAbilitySystem")))
	{
		// 현재값이 먼저 오면 옛 Max로 클램프된 뒤, 이어지는 Max 기록이 PostAttributeChange의 비례 스케일을 깨워 방금 넣은 값을 덮어쓴다.
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxHPAttribute(), Row->MaxHP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetHPAttribute(), Row->HP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxSPAttribute(), Row->MaxSP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetSPAttribute(), Row->SP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxDPAttribute(), Row->MaxDP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetDPAttribute(), Row->DP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxMPAttribute(), Row->MaxMP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMPAttribute(), Row->MP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxUPAttribute(), Row->MaxUP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetUPAttribute(), Row->UP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetATKAttribute(), Row->ATK);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetDEFAttribute(), Row->DEF);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritRateAttribute(), Row->CritRate);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritDMGAttribute(), Row->CritDMG);
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwner());

	for (const TSubclassOf<UGameplayEffect>& Effect : GrantedEffects)
	{
		if (!Effect)
		{
			continue;
		}

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1, Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1);

		ASC->GiveAbility(Spec);
	}
}

TArray<const UInputAction*> UWxAbilitySet::GetInputActions() const
{
	TArray<const UInputAction*> InputActions;
	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
	{
		const UWxAbilityBase* AbilityCDO = AbilityClass.GetDefaultObject();
		if (AbilityCDO && AbilityCDO->ActivationInputAction)
		{
			InputActions.AddUnique(AbilityCDO->ActivationInputAction.Get());
		}
	}
	return InputActions;
}
