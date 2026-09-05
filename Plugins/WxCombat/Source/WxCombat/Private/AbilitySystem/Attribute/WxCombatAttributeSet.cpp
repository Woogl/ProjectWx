// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_AddAttribute.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Effect/WxEffect_Exhaust.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Damage/WxCombatEffectContext.h"
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

			// 즉사 GE가 타격 반응을 내지 않게 Damage GE만 처리하고, 사망 처리 뒤에 호출해 사망 어빌리티의 BlockAbilitiesWithTag가 히트리액트를 막게 한다.
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

		ProcessPerfectGuard(Data, ReflectAmount);
	}
	else if (Data.EvaluatedData.Attribute == GetSPAttribute())
	{
		// 남은 양이 아니라 소모 후 결과로 지속시간을 고르므로, 0에서 또 깎여도 짧은 쪽으로 갱신되지 않는다.
		// MaxSP가 없는 아바타는 스태미나를 쓰지 않으므로 제외한다.
		// 부호 검사는 UWxEffect_Cost가 MP·UP·SP를 한 GE에 담은 탓에 0으로 실행되는 형제 모디파이어까지 막는다 — 빼면 SP를 안 쓰는 어빌리티도 걸린다.
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (Data.EvaluatedData.Magnitude < 0.f && GetMaxSP() > 0.f && ASC && ASC->IsOwnerActorAuthoritative())
		{
			UWxEffect_Exhaust::ApplyTo(ASC, GetSP() <= 0.f ? UWxEffect_Exhaust::ExhaustDuration : UWxEffect_Exhaust::ConsumeDelay);
		}
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

	const FGameplayTag ReactionTag = Data.EffectSpec.GetDynamicAssetTags().Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First();

	// GuardReact가 같은 피격 이벤트로 흡수 몽타주를 틀므로, 가드로 막히지 않는 히트는 이벤트보다 먼저 가드를 끊어야 한다.
	// 반응 라우팅은 전부 Ability.Guard로 판정한다 — 여기만 Effect.GuardReduction을 보면 둘이 어긋난 상태에서 취소를 건너뛴 채 흡수 연출이 나간다.
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Guard) && !Data.EffectSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanGuard))
	{
		const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
		ASC->CancelAbilities(&GuardAbilityTags);
	}

	// 브레이크 여부를 반응 종류에 실어 보내는 이유: 어빌리티 트리거는 RPC라 어트리뷰트 복제보다 먼저 도착해, 소유 클라가 SP를 다시 읽으면 차감 전 값을 본다.
	// 받아 줄 GuardReact가 Ability.Guard를 요구하므로, 같은 히트의 GP로 뜬 그로기가 가드를 먼저 끊었으면 일반 반응으로 보낸다.
	const bool bGuardBroken = Data.EffectSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_GuardBreak) && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Guard);
	const FGameplayTag HitEventTag = bGuardBroken
		? WxGameplayTags::Event_Hit_GuardBreak
		: WxGameplayTags::Event_Hit;
	FGameplayEventData HitEventData;
	HitEventData.EventTag = HitEventTag;
	if (ReactionTag.IsValid())
	{
		HitEventData.TargetTags.AddTag(ReactionTag);
	}
	HitEventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	HitEventData.Target = TargetActor;
	HitEventData.EventMagnitude = Damage;
	HitEventData.ContextHandle = ContextHandle;
	ASC->HandleGameplayEvent(HitEventTag, &HitEventData);

	if (SourceASC)
	{
		FGameplayEventData DamageDealtEventData;
		DamageDealtEventData.EventTag = WxGameplayTags::Event_DamageDealt;
		DamageDealtEventData.Instigator = SourceASC->GetOwnerActor();
		DamageDealtEventData.Target = TargetActor;
		DamageDealtEventData.EventMagnitude = Damage;
		DamageDealtEventData.ContextHandle = ContextHandle;
		SourceASC->HandleGameplayEvent(WxGameplayTags::Event_DamageDealt, &DamageDealtEventData);
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

	const bool bCanParry = Data.EffectSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanParry);

	// GP를 MaxGP로 되돌리면 남은 드레인 시간에 0에 닿지 않아 그로기가 스스로 풀리지 못한다.
	// 컨텍스트를 가드한 쪽으로 만들어야 이 GP가 유발하는 공격자 그로기의 원인이 공격자 자신이 되지 않는다.
	if (bCanParry && SourceASC && !SourceASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		UWxEffect_AddGP::Apply(SourceASC, ReflectAmount, ASC->MakeEffectContext());
	}

	// 컨텍스트를 함께 실어야 가드 리액션이 피격 이벤트와 같은 방식으로 원인 액터를 집는다.
	FGameplayEventData EventData;
	EventData.Instigator = SourceActor;
	EventData.Target = TargetActor;
	EventData.ContextHandle = ContextHandle;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_PerfectGuard, EventData);

	if (bCanParry && SourceActor)
	{
		FGameplayEventData ParryEventData;
		ParryEventData.EventTag = WxGameplayTags::Event_Hit_Parry;
		ParryEventData.Instigator = TargetActor;
		ParryEventData.Target = SourceActor;
		SourceASC->HandleGameplayEvent(WxGameplayTags::Event_Hit_Parry, &ParryEventData);
	}

	// 컨텍스트만 넘기면 UWxAbilitySystemGlobals가 ImpactPoint를 Location으로 채운다 — 이 큐는 그 자리에서 나이아가라와 사운드를 낸다.
	ASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_PerfectGuard, ContextHandle);
}
