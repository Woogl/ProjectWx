// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySet.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Attribute/WxCombatAttributeInitTableRow.h"
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
			Context.AddError(FText::FromString(FString::Printf(TEXT("속성 행 %s.%s를 찾을 수 없어 속성 초기화를 건너뛴다."), *GetNameSafe(AttributeInitRow.DataTable), *AttributeInitRow.RowName.ToString())));
		}
	}

	TArray<const UWxAbilityBase*> Abilities;
	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
	{
		if (!AbilityClass)
		{
			Context.AddWarning(INVTEXT("빈 어빌리티 칸이 있어 부여할 때 건너뛴다."));
			continue;
		}

		const UWxAbilityBase* Ability = AbilityClass.GetDefaultObject();
		if (Abilities.Contains(Ability))
		{
			Context.AddWarning(FText::FromString(FString::Printf(TEXT("%s가 두 번 있어 부여할 때 한 번만 준다."), *AbilityClass->GetName())));
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
				Context.AddWarning(FText::FromString(FString::Printf(TEXT("%s와 %s가 같은 입력 %s인데 발동 조건이 겹친다. 둘 다 성립하면 세트 순서대로 앞 어빌리티가 나간다."),
					*Ability.GetClass()->GetName(), *Other.GetClass()->GetName(), *Ability.ActivationInputAction->GetName())));
			}

			if (Ability.GetCooldownTags()->HasAnyExact(*Other.GetCooldownTags()) && (Ability.GetCooldownTime() != Other.GetCooldownTime() || Ability.GetMaxRecharges() != Other.GetMaxRecharges()))
			{
				Context.AddWarning(FText::FromString(FString::Printf(TEXT("%s와 %s가 같은 쿨다운 태그를 쓰는데 쿨다운 시간이나 충전 수가 다르다."),
					*Ability.GetClass()->GetName(), *Other.GetClass()->GetName())));
			}
		}
	}

	return CombineDataValidationResults(Result, Context.GetNumErrors() > NumErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif
