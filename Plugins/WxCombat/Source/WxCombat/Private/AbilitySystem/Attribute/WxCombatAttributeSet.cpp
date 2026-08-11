// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_Exhaust.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Perception/AISense_Damage.h"
#include "WxGameplayTags.h"

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
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, DP,		COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UWxCombatAttributeSet, MaxDP,	COND_None, REPNOTIFY_Always);
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
	else if (Attribute == GetMaxSPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetSPAttribute())
	{
		const float CurrentMaxSP = GetMaxSP();
		if (CurrentMaxSP > 0.f)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, CurrentMaxSP);
		}
	}
	else if (Attribute == GetMaxDPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetDPAttribute())
	{
		const float CurrentMaxDP = GetMaxDP();
		if (CurrentMaxDP > 0.f)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, CurrentMaxDP);
		}
	}
	else if (Attribute == GetMaxUPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetUPAttribute())
	{
		const float CurrentMaxUP = GetMaxUP();
		if (CurrentMaxUP > 0.f)
		{
			NewValue = FMath::Clamp(NewValue, 0.f, CurrentMaxUP);
		}
	}
	else if (Attribute == GetSPDAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetASPDAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
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

	if (Attribute == GetMaxHPAttribute() && OldValue > 0.f && NewValue > 0.f)
	{
		const float Ratio = GetHP() / OldValue;
		SetHP(NewValue * Ratio);
	}
	else if (Attribute == GetMaxSPAttribute() && OldValue > 0.f && NewValue > 0.f)
	{
		const float Ratio = GetSP() / OldValue;
		SetSP(NewValue * Ratio);
	}
	else if (Attribute == GetMaxMPAttribute() && OldValue > 0.f && NewValue > 0.f)
	{
		const float Ratio = GetMP() / OldValue;
		SetMP(NewValue * Ratio);
	}
	else if (Attribute == GetDPAttribute() && GetMaxDP() > 0.f)
	{
		if (GetDP() >= GetMaxDP() && !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
		{
			ASC->AddLooseGameplayTag(WxGameplayTags::State_Groggy, 1, EGameplayTagReplicationState::TagOnly);
		}
		else if (GetDP() <= 0.f && ASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Groggy, 1, EGameplayTagReplicationState::TagOnly);
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

			// IncomingDamage 경로로 HP가 0 이하가 됐을 때만 사망 처리한다.
			if (GetHP() <= 0.f)
			{
				UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
				if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
				{
					ASC->AddLooseGameplayTag(WxGameplayTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
				}
			}

			// AI Perception(촉각)에 보고해 가해자를 즉시 TargetActor로 인지하게 한다.
			// EventLocation으로 넘긴 가해자 위치가 그대로 Stimulus 위치가 된다.
			const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
			AActor* DamageInstigator = Context.GetInstigator();
			AActor* DamagedActor = GetOwningActor();
			if (DamageInstigator && DamagedActor)
			{
				const FVector HitLocation = Context.GetHitResult() ? FVector(Context.GetHitResult()->ImpactPoint) : DamagedActor->GetActorLocation();
				UAISense_Damage::ReportDamageEvent(DamagedActor, DamagedActor, DamageInstigator, Damage, DamageInstigator->GetActorLocation(), HitLocation);
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
	else if (Data.EvaluatedData.Attribute == GetSPAttribute())
	{
		SetSP(FMath::Clamp(GetSP(), 0.f, GetMaxSP()));

		// SP를 깎는 GE는 질주 소모든 회피 코스트든 가드 피격이든 모두 이 지점을 지난다.
		// 남은 양이 아니라 소모 후 결과로 지속시간을 고르므로, 0에서 또 깎여도 짧은 쪽으로 갱신되지 않는다.
		// MaxSP가 없는 아바타는 스태미나를 쓰지 않으므로 제외한다.
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (Data.EvaluatedData.Magnitude < 0.f && GetMaxSP() > 0.f && ASC && ASC->IsOwnerActorAuthoritative())
		{
			UWxEffect_Exhaust::ApplyTo(ASC, GetSP() <= 0.f ? UWxEffect_Exhaust::ExhaustDuration : UWxEffect_Exhaust::ConsumeDelay);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetUPAttribute())
	{
		SetUP(FMath::Clamp(GetUP(), 0.f, GetMaxUP()));
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

void UWxCombatAttributeSet::OnRep_SP(const FGameplayAttributeData& OldSP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, SP, OldSP);
}

void UWxCombatAttributeSet::OnRep_MaxSP(const FGameplayAttributeData& OldMaxSP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxSP, OldMaxSP);
}

void UWxCombatAttributeSet::OnRep_DP(const FGameplayAttributeData& OldDP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, DP, OldDP);
}

void UWxCombatAttributeSet::OnRep_MaxDP(const FGameplayAttributeData& OldMaxDP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxDP, OldMaxDP);
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