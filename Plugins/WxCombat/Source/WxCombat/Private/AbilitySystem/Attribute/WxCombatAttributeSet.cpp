// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_AddDP.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Effect/WxEffect_Exhaust.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Damage/WxCombatEffectContext.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
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

	NewValue = FMath::Max(NewValue, 0.f);

	if (Attribute == GetHPAttribute())
	{
		const float CurrentMaxHP = GetMaxHP();
		if (CurrentMaxHP > 0.f)
		{
			NewValue = FMath::Min(NewValue, CurrentMaxHP);
		}
	}
	else if (Attribute == GetMPAttribute())
	{
		const float CurrentMaxMP = GetMaxMP();
		if (CurrentMaxMP > 0.f)
		{
			NewValue = FMath::Min(NewValue, CurrentMaxMP);
		}
	}
	else if (Attribute == GetSPAttribute())
	{
		const float CurrentMaxSP = GetMaxSP();
		if (CurrentMaxSP > 0.f)
		{
			NewValue = FMath::Min(NewValue, CurrentMaxSP);
		}
	}
	else if (Attribute == GetDPAttribute())
	{
		const float CurrentMaxDP = GetMaxDP();
		if (CurrentMaxDP > 0.f)
		{
			NewValue = FMath::Min(NewValue, CurrentMaxDP);
		}
	}
	else if (Attribute == GetUPAttribute())
	{
		const float CurrentMaxUP = GetMaxUP();
		if (CurrentMaxUP > 0.f)
		{
			NewValue = FMath::Min(NewValue, CurrentMaxUP);
		}
	}
}

void UWxCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	NewValue = FMath::Max(NewValue, 0.f);
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
		// 그로기 진입만 알리고 해제는 관여하지 않는다 — 어빌리티가 DP를 직접 보고 스스로 끝낸다.
		if (GetDP() >= GetMaxDP() && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
		{
			FGameplayEventData EventData;
			EventData.EventTag = WxGameplayTags::Event_Groggy;
			EventData.Instigator = GetOwningActor();
			EventData.Target = EventData.Instigator;
			ASC->HandleGameplayEvent(WxGameplayTags::Event_Groggy, &EventData);
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
			// 사망 표식(Ability.Death)은 사망 어빌리티가 활성 동안 들고 있으므로, 여기서는 발동만 알린다.
			if (GetHP() <= 0.f)
			{
				UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
				if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
				{
					FGameplayEventData EventData;
					EventData.EventTag = WxGameplayTags::Event_Death;
					EventData.Instigator = GetOwningActor();
					EventData.Target = EventData.Instigator;
					ASC->HandleGameplayEvent(WxGameplayTags::Event_Death, &EventData);
				}
			}

			// 화상도 IncomingDamage를 지나므로 GE 종류로 걸러야 틱마다 반응과 연출이 다시 나가지 않는다.
			// 사망 처리 뒤라야 Ability.Death가 붙은 상태가 되어 히트리액트가 그 태그로 차단된다.
			if (Data.EffectSpec.Def && Data.EffectSpec.Def->IsA<UWxEffect_Damage>())
			{
				ProcessDamageTaken(Data, Damage);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingReflectAttribute())
	{
		const float ReflectAmount = GetIncomingReflect();
		SetIncomingReflect(0.f);

		// 반사량이 0이어도 퍼펙트 가드 성공 자체는 알려야 한다.
		ProcessPerfectGuard(Data, ReflectAmount);
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
	else if (Data.EvaluatedData.Attribute == GetDPAttribute())
	{
		SetDP(FMath::Clamp(GetDP(), 0.f, GetMaxDP()));
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

void UWxCombatAttributeSet::ProcessDamageTaken(const FGameplayEffectModCallbackData& Data, float Damage)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext();
	const FGameplayEffectContext* RawContext = ContextHandle.Get();
	if (!ASC || !RawContext || RawContext->GetScriptStruct() != FWxCombatEffectContext::StaticStruct())
	{
		return;
	}

	const FWxCombatEffectContext& CombatContext = static_cast<const FWxCombatEffectContext&>(*RawContext);

	AActor* TargetActor = GetOwningActor();
	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();

	UWxEffect_RecoverResource::ApplyTo(SourceASC,
		Data.EffectSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, false, 0.f),
		Data.EffectSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, false, 0.f));

	// 공격이 요청한 반응 태그는 스펙에 실려 온다 — 바로 아래 Damage.Unblockable을 읽는 것과 같은 자리다.
	FGameplayTag HitReactTag = Data.EffectSpec.GetDynamicAssetTags().Filter(FGameplayTagContainer(WxGameplayTags::Event_HitReact)).First();

	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Effect_Guard))
	{
		if (Data.EffectSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable))
		{
			// HitReact 어빌리티가 Effect.Guard로 차단되므로 반응 이벤트보다 먼저 가드를 끊어야 한다.
			const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
			ASC->CancelAbilities(&GuardAbilityTags);
		}
		else if (!HitReactTag.IsValid())
		{
			// 명시 태그가 없는 평타 피격에도 가드 흡수 애니메이션은 나와야 한다.
			HitReactTag = WxGameplayTags::Event_HitReact_Normal;
		}
	}

	// 죽는 히트는 Ability.Death가 이미 붙어 있어 히트리액트가 그 태그로 차단된다.
	// 히트리액트를 재생했다 사망으로 끊는 대신 곧장 사망으로 가는 쪽을 택했다.
	if (HitReactTag.IsValid())
	{
		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		EventData.Target = TargetActor;
		EventData.EventMagnitude = Damage;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, HitReactTag, EventData);
	}

	// 플로터는 큐 노티파이가 대상 액터 위치에서 직접 띄우므로 Location을 채우지 않는다.
	FGameplayCueParameters CueParams;
	CueParams.EffectContext = ContextHandle;
	CueParams.RawMagnitude = Damage;
	if (CombatContext.IsCritical())
	{
		CueParams.AggregatedSourceTags.AddTag(WxGameplayTags::Damage_Critical);
	}

	ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_DamageFloater, CueParams);
}

void UWxCombatAttributeSet::ProcessPerfectGuard(const FGameplayEffectModCallbackData& Data, float ReflectAmount)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const FGameplayEffectContextHandle ContextHandle = Data.EffectSpec.GetContext();

	AActor* TargetActor = GetOwningActor();
	UAbilitySystemComponent* SourceASC = ContextHandle.GetInstigatorAbilitySystemComponent();
	AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;

	// 대상의 DP 가산과 같은 이유로 이미 그로기인 공격자에겐 반사하지 않는다.
	// DP를 MaxDP로 되돌리면 남은 드레인 시간으로는 0에 닿지 못해 그로기가 안전 타이머까지 늘어진다.
	if (SourceASC && !SourceASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		UWxEffect_AddDP::ApplyTo(SourceASC, ReflectAmount);
	}

	FGameplayEventData EventData;
	EventData.Instigator = SourceActor;
	EventData.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_PerfectGuard, EventData);

	if (SourceActor && Data.EffectSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_ParryHitReact))
	{
		FGameplayEventData ParryEventData;
		ParryEventData.Instigator = TargetActor;
		ParryEventData.Target = SourceActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceActor, WxGameplayTags::Event_HitReact_Parry, ParryEventData);
	}

	// 컨텍스트만 넘기면 UWxAbilitySystemGlobals가 ImpactPoint를 Location으로 채운다 — 이 큐는 그 자리에서 나이아가라와 사운드를 낸다.
	ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_PerfectGuard, ContextHandle);
}
