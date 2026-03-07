// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/WxAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UWxAttributeSet::UWxAttributeSet() {}

void UWxAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, HP,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, MaxHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, MP,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, MaxMP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxAttributeSet, ATK,   COND_None, REPNOTIFY_Always);
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
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
	}
	else if (Attribute == GetMPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMP());
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

void UWxAttributeSet::OnRep_SPD(const FGameplayAttributeData& OldSPD)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxAttributeSet, SPD, OldSPD);
}
