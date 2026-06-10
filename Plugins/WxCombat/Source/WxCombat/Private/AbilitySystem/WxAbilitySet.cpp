// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySet.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Attribute/WxCombatAttributeInitTableRow.h"

void FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem(UWxAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : EffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	AbilitySpecHandles.Reset();
	EffectHandles.Reset();
}

void UWxAbilitySet::GiveToAbilitySystem(UWxAbilitySystemComponent* ASC, FWxAbilitySetGrantedHandles* OutHandles) const
{
	if (!ASC)
	{
		return;
	}
	
	// 데이터테이블에서 어트리뷰트 초기값 설정
	if (const FWxCombatAttributeInitTableRow* Row = AttributeInitRow.GetRow<FWxCombatAttributeInitTableRow>(TEXT("WxAbilitySet::GiveToAbilitySystem")))
	{
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetHPAttribute(), Row->HP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxHPAttribute(), Row->MaxHP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetSPAttribute(), Row->SP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxSPAttribute(), Row->MaxSP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetDPAttribute(), Row->DP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxDPAttribute(), Row->MaxDP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMPAttribute(), Row->MP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxMPAttribute(), Row->MaxMP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetUPAttribute(), Row->UP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxUPAttribute(), Row->MaxUP);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetATKAttribute(), Row->ATK);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetDEFAttribute(), Row->DEF);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritRateAttribute(), Row->CritRate);
		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritDMGAttribute(), Row->CritDMG);
	}

	// Effect 적용
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
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			if (OutHandles)
			{
				OutHandles->EffectHandles.Add(Handle);
			}
		}
	}

	// Ability 부여
	for (const FWxAbilitySet_GameplayAbility& AbilityToGrant : GrantedAbilities)
	{
		if (!AbilityToGrant.Ability)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityToGrant.Ability, 1);

		// 입력으로 발동하는 어빌리티는 InputTag를 Spec 소스 태그로 추가해 ASC 입력 매칭의 키로 쓴다.
		if (AbilityToGrant.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);
		}

		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		if (OutHandles)
		{
			OutHandles->AbilitySpecHandles.Add(Handle);
		}
	}
}
