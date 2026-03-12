// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "WxGameplayTags.h"

UWxAttributeSet::UWxAttributeSet() {}

void UWxAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, HP,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, MaxHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, MP,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, MaxMP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, ATK,   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, DEF,   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, SPD,   COND_None, REPNOTIFY_Always);
}

void UWxAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHPAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetMaxMPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetHPAttribute())
	{
		const float CurrentMaxHP = GetMaxHP();
		if (CurrentMaxHP > 0.f)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, CurrentMaxHP);
		}
	}
	else if (Attribute == GetMPAttribute())
	{
		const float CurrentMaxMP = GetMaxMP();
		if (CurrentMaxMP > 0.f)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, CurrentMaxMP);
		}
	}
}

void UWxAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (Damage > 0.f)
		{
			SetHP(FMath::Max(GetHP() - Damage, 0.f));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHPAttribute())
	{
		SetHP(FMath::Clamp(GetHP(), 0.f, GetMaxHP()));
	}
	else if (Data.EvaluatedData.Attribute == GetMPAttribute())
	{
		SetMP(FMath::Clamp(GetMP(), 0.f, GetMaxMP()));
	}

	// IncomingDamage에 의해 HP가 0 이하가 된 경우에만 사망 처리
	// 초기화 시 순서 문제로 오발동되지 않도록 IncomingDamage 분기 내에서만 체크
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute() && GetHP() <= 0.f)
	{
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
		{
			ASC->AddLooseGameplayTag(WxGameplayTags::State_Dead);
		}
	}
}

void UWxAttributeSet::OnRep_HP(const FGameplayAttributeData& OldHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, HP, OldHP);
}

void UWxAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, MaxHP, OldMaxHP);
}

void UWxAttributeSet::OnRep_MP(const FGameplayAttributeData& OldMP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, MP, OldMP);
}

void UWxAttributeSet::OnRep_MaxMP(const FGameplayAttributeData& OldMaxMP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, MaxMP, OldMaxMP);
}

void UWxAttributeSet::OnRep_ATK(const FGameplayAttributeData& OldATK)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, ATK, OldATK);
}

void UWxAttributeSet::OnRep_DEF(const FGameplayAttributeData& OldDEF)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, DEF, OldDEF);
}

void UWxAttributeSet::OnRep_SPD(const FGameplayAttributeData& OldSPD)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, SPD, OldSPD);
}
