// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxCombatAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "WxGameplayTags.h"

void UWxCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, HP,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MP,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxMP,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, DP,     COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxDP,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, ATK,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, DEF,   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, SPD,      COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, CritRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, CritDMG,  COND_None, REPNOTIFY_Always);
}

void UWxCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
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
	else if (Attribute == GetMaxDPAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetDPAttribute())
	{
		const float CurrentMaxDP = GetMaxDP();
		if (CurrentMaxDP > 0.f)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, CurrentMaxDP);
		}
	}
}

void UWxCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (Damage > 0.f)
		{
			SetHP(FMath::Max(GetHP() - Damage, 0.f));

			// IncomingDamage에 의해 HP가 0 이하가 된 경우에만 사망 처리
			if (GetHP() <= 0.f)
			{
				UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
				if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
				{
					ASC->AddLooseGameplayTag(WxGameplayTags::State_Dead);
				}
			}
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
	else if (Data.EvaluatedData.Attribute == GetDPAttribute())
	{
		SetDP(FMath::Clamp(GetDP(), 0.f, GetMaxDP()));
	}
}

void UWxCombatAttributeSet::OnRep_HP(const FGameplayAttributeData& OldHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, HP, OldHP);
}

void UWxCombatAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxHP, OldMaxHP);
}

void UWxCombatAttributeSet::OnRep_MP(const FGameplayAttributeData& OldMP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MP, OldMP);
}

void UWxCombatAttributeSet::OnRep_MaxMP(const FGameplayAttributeData& OldMaxMP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxMP, OldMaxMP);
}

void UWxCombatAttributeSet::OnRep_ATK(const FGameplayAttributeData& OldATK)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, ATK, OldATK);
}

void UWxCombatAttributeSet::OnRep_DEF(const FGameplayAttributeData& OldDEF)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, DEF, OldDEF);
}

void UWxCombatAttributeSet::OnRep_SPD(const FGameplayAttributeData& OldSPD)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, SPD, OldSPD);
}

void UWxCombatAttributeSet::OnRep_CritRate(const FGameplayAttributeData& OldCritRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, CritRate, OldCritRate);
}

void UWxCombatAttributeSet::OnRep_CritDMG(const FGameplayAttributeData& OldCritDMG)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, CritDMG, OldCritDMG);
}

void UWxCombatAttributeSet::OnRep_DP(const FGameplayAttributeData& OldDP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, DP, OldDP);
}

void UWxCombatAttributeSet::OnRep_MaxDP(const FGameplayAttributeData& OldMaxDP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxDP, OldMaxDP);
}
