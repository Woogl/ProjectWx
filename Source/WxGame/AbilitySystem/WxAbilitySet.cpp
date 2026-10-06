// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySet.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "AbilitySystem/Attributes/WxCombatAttributeSet.h"
#include "AbilitySystem/Attributes/WxCombatAttributeInitTableRow.h"
#include "GameplayAbilitySpec.h"
#include "InputAction.h"
#include "Misc/DataValidation.h"

void UWxAbilitySet::GiveToAbilitySystem(UWxAbilitySystemComponent* ASC) const
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	if (const FWxCombatAttributeInitTableRow* Row = AttributeInitRow.GetRow<FWxCombatAttributeInitTableRow>(ANSI_TO_TCHAR(__FUNCTION__)))
	{
		// 현재값이 먼저 오면 옛 Max로 클램프된 뒤, 이어지는 Max 기록이 PostAttributeChange의 비례 스케일을 깨워 방금 넣은 값을 덮어쓴다.
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxHPAttribute(), Row->MaxHP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetHPAttribute(), Row->HP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxSPAttribute(), Row->MaxSP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetSPAttribute(), Row->SP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxGPAttribute(), Row->MaxGP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetGPAttribute(), Row->GP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxMPAttribute(), Row->MaxMP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMPAttribute(), Row->MP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxUPAttribute(), Row->MaxUP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetUPAttribute(), Row->UP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetATKAttribute(), Row->ATK);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetDEFAttribute(), Row->DEF);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritRateAttribute(), Row->CritRate);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritDMGAttribute(), Row->CritDMG);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMOVAttribute(), Row->MOV);
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

	GiveAbilitiesToAbilitySystem(ASC);
}

void UWxAbilitySet::GiveAbilitiesToAbilitySystem(UWxAbilitySystemComponent* ASC) const
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
	{
		if (!AbilityClass || ASC->FindAbilitySpecFromClass(AbilityClass))
		{
			continue;
		}

		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

void UWxAbilitySet::AppendInputActions(TArray<const UInputAction*>& OutInputActions) const
{
	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
	{
		const UWxAbilityBase* AbilityCDO = AbilityClass.GetDefaultObject();
		if (AbilityCDO && AbilityCDO->ActivationInputAction)
		{
			OutInputActions.AddUnique(AbilityCDO->ActivationInputAction.Get());
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UWxAbilitySet::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	const uint32 NumErrors = Context.GetNumErrors();

	// 속성 행은 비워 둘 수 있다.
	if (!AttributeInitRow.IsNull())
	{
		const FWxCombatAttributeInitTableRow* AttributeRow = AttributeInitRow.DataTable ? AttributeInitRow.DataTable->FindRow<FWxCombatAttributeInitTableRow>(AttributeInitRow.RowName, GetName(), false) : nullptr;
		if (!AttributeRow)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Attribute row %s.%s was not found, so attribute initialization is skipped."), *GetNameSafe(AttributeInitRow.DataTable), *AttributeInitRow.RowName.ToString())));
		}
	}

	TArray<const UWxAbilityBase*> Abilities;
	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
	{
		if (!AbilityClass)
		{
			Context.AddWarning(INVTEXT("An empty ability slot is skipped when granting."));
			continue;
		}

		const UWxAbilityBase* Ability = AbilityClass.GetDefaultObject();
		if (Abilities.Contains(Ability))
		{
			Context.AddWarning(FText::FromString(FString::Printf(TEXT("%s is listed twice and is granted only once."), *AbilityClass->GetName())));
			continue;
		}
		Abilities.Add(Ability);
	}

	for (int32 Index = 0; Index < Abilities.Num(); ++Index)
	{
		const UWxAbilityBase& Ability = *Abilities[Index];
		for (int32 OtherIndex = Index + 1; OtherIndex < Abilities.Num(); ++OtherIndex)
		{
			const UWxAbilityBase& Other = *Abilities[OtherIndex];

			if (Ability.ActivationInputAction && Ability.ActivationInputAction == Other.ActivationInputAction && !Ability.IsActivationExclusive(Other))
			{
				Context.AddWarning(FText::FromString(FString::Printf(TEXT("%s and %s share the input %s and their activation conditions overlap. When both are met, the one listed first in the set activates."),
					*Ability.GetClass()->GetName(), *Other.GetClass()->GetName(), *Ability.ActivationInputAction->GetName())));
			}
		}
	}

	return CombineDataValidationResults(Result, Context.GetNumErrors() > NumErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif
