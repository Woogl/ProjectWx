// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
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

	const UGameplayAbility* AbilityCDO = GetClass()->GetDefaultObject<UGameplayAbility>();
	const float WorldTime = ASC->GetWorld()->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = UWxEffect_Cooldown::StaticClass();

	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC->GetActiveEffects(Query))
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle))
		{
			if (ActiveGE->Spec.GetEffectContext().GetAbility() == AbilityCDO)
			{
				const float TimeRemaining = (ActiveGE->StartWorldTime + ActiveGE->Spec.GetDuration()) - WorldTime;
				return FMath::CeilToInt32((TimeRemaining - KINDA_SMALL_NUMBER) / CooldownTime) < MaxRecharges;
			}
		}
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

	// 기존 쿨다운 GE의 잔여시간을 구한 뒤 제거한다
	float Remaining = 0.f;
	const UGameplayAbility* AbilityCDO = GetClass()->GetDefaultObject<UGameplayAbility>();
	const float WorldTime = ASC->GetWorld()->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = UWxEffect_Cooldown::StaticClass();

	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC->GetActiveEffects(Query))
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle))
		{
			if (ActiveGE->Spec.GetEffectContext().GetAbility() == AbilityCDO)
			{
				Remaining = FMath::Max(0.f, (ActiveGE->StartWorldTime + ActiveGE->Spec.GetDuration()) - WorldTime);
				ASC->RemoveActiveGameplayEffect(ActiveHandle);
				break;
			}
		}
	}

	// 새 쿨다운 GE 적용. Duration = 잔여시간 + CooldownTime (직렬 충전 회복)
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, Remaining + CooldownTime);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
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

	// 엔진 ApplyCost(ApplyGameplayEffectToOwner)는 전달받은 GE의 GetClass() CDO로 스펙을 만들어,
	// 런타임에 모디파이어를 채운 CostEffect 인스턴스가 무시된다(UWxEffect_Cost CDO는 모디파이어가 비어 있다).
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
