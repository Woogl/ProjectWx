// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "WxAbilityComponent.h"
#include "WxGameplayTags.h"

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

const UWxAbilityComponent* UWxAbilityBase::FindComponent(TSubclassOf<UWxAbilityComponent> ComponentClass) const
{
	if (!ComponentClass)
	{
		return nullptr;
	}

	for (const UWxAbilityComponent* Component : Components)
	{
		if (Component && Component->IsA(ComponentClass))
		{
			return Component;
		}
	}
	return nullptr;
}

float UWxAbilityBase::GetMontagePlayRate() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return 1.f;
	}

	const UWxCombatAttributeSet* AttrSet = ASC->GetSet<UWxCombatAttributeSet>();
	if (!AttrSet)
	{
		return 1.f;
	}

	return FMath::Max(AttrSet->GetASPD(), 0.01f);
}

void UWxAbilityBase::StartRecovery()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 자기 자신이 건 차단만 정확히 해제한다(전역 스냅샷 아님). ref-count는 0에서 클램프되므로 EndAbility의 중복 해제도 무해.
		ASC->UnBlockAbilitiesWithTags(BlockAbilitiesWithTag);
	}
}

#if WITH_EDITOR
bool UWxAbilityBase::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}
	
	if (InProperty)
	{
		const FName PropertyName = InProperty->GetFName();
		const bool bHasDataRow = !AbilityDataRow.IsNull();

		static const FName CooldownGEName = TEXT("CooldownGameplayEffectClass");
		static const FName CostGEName = TEXT("CostGameplayEffectClass");
		if (PropertyName == CooldownGEName || PropertyName == CostGEName)
		{
			if (bHasDataRow)
			{
				return false;
			}
		}

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, CooldownTime)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, MaxRecharges))
		{
			if (CooldownGameplayEffectClass || bHasDataRow)
			{
				return false;
			}
		}

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, MPCost)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, UPCost))
		{
			if (CostGameplayEffectClass || bHasDataRow)
			{
				return false;
			}
		}
	}
	
	return true;
}

void UWxAbilityBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, AbilityDataRow))
	{
		if (!AbilityDataRow.IsNull())
		{
			CooldownGameplayEffectClass = nullptr;
			CostGameplayEffectClass = nullptr;
		}
	}
}

#endif

void UWxAbilityBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (const FWxAbilityTableRow* Row = AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("WxAbilityBase::OnGiveAbility")))
	{
		ApplyAbilityTableRow(*Row);

		UWxAbilityBase* CDO = GetClass()->GetDefaultObject<UWxAbilityBase>();
		if (CDO != this)
		{
			CDO->ApplyAbilityTableRow(*Row);
		}
	}

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

UGameplayEffect* UWxAbilityBase::GetCooldownGameplayEffect() const
{
	if (CooldownGameplayEffectClass)
	{
		return Super::GetCooldownGameplayEffect();
	}

	if (CooldownTime <= 0.f)
	{
		return nullptr;
	}

	if (!CooldownEffect)
	{
		CooldownEffect = NewObject<UWxEffect_Cooldown>(const_cast<UWxAbilityBase*>(this), TEXT("CooldownEffect"));
	}

	CooldownEffect->StackLimitCount = MaxRecharges;
	return CooldownEffect;
}

bool UWxAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (CooldownGameplayEffectClass)
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	if (CooldownTime <= 0.f)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return true;
	}

	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	if (QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration) >= MaxRecharges)
	{
		// 엔진 순정 CheckCooldown과 동일하게 실패 사유 태그를 채워 OnAbilityFailed 파이프라인(실패 피드백 UI 등)에 전달한다
		if (OptionalRelevantTags)
		{
			const FGameplayTag& FailCooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
			if (FailCooldownTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailCooldownTag);
			}
		}
		return false;
	}

	return true;
}

void UWxAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownGameplayEffectClass)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	if (CooldownTime <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	// 소모한 충전 1개당 GE 1개를 새로 적용하고, 기존 GE는 제거하지 않고 자연 만료에 맡긴다.
	// (UE 5.7부터 비-authority의 RemoveActiveGameplayEffect가 거부되므로, 예측 커밋 중 기존 GE를 제거 후 재적용하는 방식은 쓸 수 없다)
	// 새 GE의 Duration = 가장 늦은 기존 만료까지 잔여시간 + CooldownTime 으로 직렬 충전 회복이 된다.
	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration);

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, LongestRemaining + CooldownTime);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

float UWxAbilityBase::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownGameplayEffectClass)
	{
		return Super::GetCooldownTimeRemaining(ActorInfo);
	}

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, TimeRemaining, Duration);
	}
	return TimeRemaining;
}

void UWxAbilityBase::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const
{
	if (CooldownGameplayEffectClass)
	{
		Super::GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, CooldownDuration);
		return;
	}

	TimeRemaining = 0.f;
	CooldownDuration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, TimeRemaining, CooldownDuration);
	}
}

UGameplayEffect* UWxAbilityBase::GetCostGameplayEffect() const
{
	if (CostGameplayEffectClass)
	{
		return Super::GetCostGameplayEffect();
	}

	if (MPCost <= 0.f && UPCost <= 0.f)
	{
		return nullptr;
	}

	if (!CostEffect)
	{
		CostEffect = NewObject<UWxEffect_Cost>(const_cast<UWxAbilityBase*>(this), TEXT("CostEffect"));
	}

	CostEffect->Modifiers.Reset();

	if (MPCost > 0.f)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = UWxCombatAttributeSet::GetMPAttribute();
		Modifier.ModifierOp = EGameplayModOp::AddBase;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-MPCost));
		CostEffect->Modifiers.Add(Modifier);
	}

	if (UPCost > 0.f)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = UWxCombatAttributeSet::GetUPAttribute();
		Modifier.ModifierOp = EGameplayModOp::AddBase;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-UPCost));
		CostEffect->Modifiers.Add(Modifier);
	}

	return CostEffect;
}

void UWxAbilityBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CostGameplayEffectClass)
	{
		Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
		return;
	}

	const UGameplayEffect* CostGE = GetCostGameplayEffect();
	if (!CostGE)
	{
		return;
	}

	// 엔진 ApplyCost(ApplyGameplayEffectToOwner)는 전달받은 GE의 GetClass() CDO로 스펙을 만들어, 런타임에 모디파이어를 채운 CostEffect 인스턴스가 무시된다(UWxEffect_Cost CDO는 모디파이어가 비어 있다).
	// 인스턴스 Def로 직접 스펙을 만들어 적용한다. 권한/예측 처리는 ApplyGameplayEffectSpecToOwner가 담당한다.
	FGameplayEffectSpecHandle SpecHandle(new FGameplayEffectSpec(CostGE, MakeEffectContext(Handle, ActorInfo), GetAbilityLevel(Handle, ActorInfo)));
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

void UWxAbilityBase::ApplyAbilityTableRow(const FWxAbilityTableRow& Row)
{
	CooldownTime = Row.CooldownTime;
	MaxRecharges = FMath::Max(1, Row.MaxRecharges);
	MPCost = Row.MPCost;
	UPCost = Row.UPCost;
}

void UWxAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	for (const FWxAbilityEffect& Effect : OnActivateEffects)
	{
		if (Effect.EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Effect.EffectClass, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				for (const auto& [Tag, Value] : Effect.SetByCallers)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(Tag, Value);
				}
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			}
		}
	}
}

int32 UWxAbilityBase::QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const
{
	OutLongestRemaining = 0.f;
	OutLongestDuration = 0.f;

	const UGameplayAbility* AbilityCDO = GetClass()->GetDefaultObject<UGameplayAbility>();
	const float WorldTime = ASC.GetWorld()->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = UWxEffect_Cooldown::StaticClass();

	int32 ActiveCount = 0;
	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC.GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* ActiveGE = ASC.GetActiveGameplayEffect(ActiveHandle);
		if (!ActiveGE || ActiveGE->Spec.GetEffectContext().GetAbility() != AbilityCDO)
		{
			continue;
		}

		// 만료됐지만 아직 제거되지 않은 GE(클라이언트는 제거가 리플리케이션으로 도착할 때까지 지연됨)는 회복된 충전으로 취급한다
		const float Remaining = (ActiveGE->StartWorldTime + ActiveGE->Spec.GetDuration()) - WorldTime;
		if (Remaining <= 0.f)
		{
			continue;
		}

		++ActiveCount;
		if (Remaining > OutLongestRemaining)
		{
			OutLongestRemaining = Remaining;
			OutLongestDuration = ActiveGE->Spec.GetDuration();
		}
	}

	return ActiveCount;
}
