// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "WxGameplayTags.h"

const UWxCombatAttributeSet::FWxMaxAttributePair* UWxCombatAttributeSet::FindMaxAttributePair(const FGameplayAttribute& Attribute)
{
	static const FWxMaxAttributePair Pairs[] =
	{
		{GetHPAttribute(), GetMaxHPAttribute()},
		{GetSPAttribute(), GetMaxSPAttribute()},
		{GetGPAttribute(), GetMaxGPAttribute()},
		{GetMPAttribute(), GetMaxMPAttribute()},
		{GetUPAttribute(), GetMaxUPAttribute()}
	};

	for (const FWxMaxAttributePair& Pair : Pairs)
	{
		if (Pair.Attribute == Attribute || Pair.MaxAttribute == Attribute)
		{
			return &Pair;
		}
	}

	return nullptr;
}

UWxCombatAttributeSet::UWxCombatAttributeSet()
{
	InitSPD(1.f);
	InitASPD(1.f);
}

void UWxCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, HP,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxHP,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, SP,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxSP,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, GP,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxGP,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MP,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxMP,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, UP,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxUP,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, ATK,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, DEF,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, CritRate,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, CritDMG,	COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, SPD,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, ASPD,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, GuardReductionScale, COND_None, REPNOTIFY_Always);
}

void UWxCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	NewValue = ClampAttributeValue(Attribute, NewValue);
}

void UWxCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	NewValue = ClampAttributeValue(Attribute, NewValue);
}

void UWxCombatAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 이 훅은 클라이언트의 복제 수신 경로에서도 호출된다. 아래 파생 갱신은 서버가 정해 복제하므로 권위 측에서만 실행한다.
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	AdjustCurrentAttributeForMaxChange(Attribute, OldValue, NewValue);
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
			SetHP(GetHP() - Damage);

			// 사망 표식(Ability.Death)은 사망 어빌리티가 활성 동안 들고 있으므로, 여기서는 발동만 알린다.
			if (GetHP() <= 0.f)
			{
				UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
				if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
				{
					FGameplayEventData EventData;
					EventData.EventTag = WxGameplayTags::Event_Death;
					EventData.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
					EventData.Target = GetOwningActor();
					ASC->HandleGameplayEvent(WxGameplayTags::Event_Death, &EventData);
				}
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingReflectAttribute())
	{
		SetIncomingReflect(0.f);
	}
	else if (Data.EvaluatedData.Attribute == GetGPAttribute() && GetMaxGP() > 0.f)
	{
		// 그로기 진입만 알리고 해제는 관여하지 않는다 — 어빌리티가 GP를 직접 보고 스스로 끝낸다.
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (GetGP() >= GetMaxGP() && ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
		{
			FGameplayEventData EventData;
			EventData.EventTag = WxGameplayTags::Event_Groggy;
			EventData.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
			EventData.Target = GetOwningActor();
			ASC->HandleGameplayEvent(WxGameplayTags::Event_Groggy, &EventData);
		}
	}
}

float UWxCombatAttributeSet::ClampAttributeValue(const FGameplayAttribute& Attribute, float NewValue) const
{
	const float MinimumValue = Attribute == GetASPDAttribute() ? 0.001f : 0.f;
	NewValue = FMath::Max(NewValue, MinimumValue);

	const FWxMaxAttributePair* Pair = FindMaxAttributePair(Attribute);
	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (Pair && Pair->Attribute == Attribute && ASC)
	{
		NewValue = FMath::Min(NewValue, ASC->GetNumericAttribute(Pair->MaxAttribute));
	}

	return NewValue;
}

void UWxCombatAttributeSet::AdjustCurrentAttributeForMaxChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	const FWxMaxAttributePair* Pair = FindMaxAttributePair(Attribute);
	if (!Pair || Pair->MaxAttribute != Attribute)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	float AdjustedValue = ASC->GetNumericAttribute(Pair->Attribute);
	if (OldValue > 0.f)
	{
		AdjustedValue *= NewValue / OldValue;
	}

	ASC->SetNumericAttributeBase(Pair->Attribute, AdjustedValue);
}

void UWxCombatAttributeSet::OnRep_HP(const FGameplayAttributeData& OldHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, HP, OldHP);
}

void UWxCombatAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxHP, OldMaxHP);
}

void UWxCombatAttributeSet::OnRep_SP(const FGameplayAttributeData& OldSP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, SP, OldSP);
}

void UWxCombatAttributeSet::OnRep_MaxSP(const FGameplayAttributeData& OldMaxSP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxSP, OldMaxSP);
}

void UWxCombatAttributeSet::OnRep_GP(const FGameplayAttributeData& OldGP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, GP, OldGP);
}

void UWxCombatAttributeSet::OnRep_MaxGP(const FGameplayAttributeData& OldMaxGP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxGP, OldMaxGP);
}

void UWxCombatAttributeSet::OnRep_MP(const FGameplayAttributeData& OldMP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MP, OldMP);
}

void UWxCombatAttributeSet::OnRep_MaxMP(const FGameplayAttributeData& OldMaxMP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxMP, OldMaxMP);
}

void UWxCombatAttributeSet::OnRep_UP(const FGameplayAttributeData& OldUP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, UP, OldUP);
}

void UWxCombatAttributeSet::OnRep_MaxUP(const FGameplayAttributeData& OldMaxUP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxUP, OldMaxUP);
}

void UWxCombatAttributeSet::OnRep_ATK(const FGameplayAttributeData& OldATK)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, ATK, OldATK);
}

void UWxCombatAttributeSet::OnRep_DEF(const FGameplayAttributeData& OldDEF)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, DEF, OldDEF);
}

void UWxCombatAttributeSet::OnRep_CritRate(const FGameplayAttributeData& OldCritRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, CritRate, OldCritRate);
}

void UWxCombatAttributeSet::OnRep_CritDMG(const FGameplayAttributeData& OldCritDMG)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, CritDMG, OldCritDMG);
}

void UWxCombatAttributeSet::OnRep_SPD(const FGameplayAttributeData& OldSPD)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, SPD, OldSPD);
}

void UWxCombatAttributeSet::OnRep_ASPD(const FGameplayAttributeData& OldASPD)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, ASPD, OldASPD);
}

void UWxCombatAttributeSet::OnRep_GuardReductionScale(const FGameplayAttributeData& OldGuardReductionScale)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, GuardReductionScale, OldGuardReductionScale);
}
