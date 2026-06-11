// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_CostMP.h"
#include "AbilitySystem/Effect/WxEffect_CostUP.h"
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
			if (bHasDataRow)
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

bool UWxAbilityBase::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (MPCost <= 0.f && UPCost <= 0.f)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	const UWxCombatAttributeSet* AttrSet = ASC->GetSet<UWxCombatAttributeSet>();
	if (!AttrSet)
	{
		return false;
	}

	if (MPCost > 0.f && AttrSet->GetMP() < MPCost)
	{
		return false;
	}

	if (UPCost > 0.f && AttrSet->GetUP() < UPCost)
	{
		return false;
	}

	return true;
}

void UWxAbilityBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (MPCost > 0.f)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_CostMP::StaticClass(), GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Cost, -MPCost);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	if (UPCost > 0.f)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_CostUP::StaticClass(), GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Cost, -UPCost);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
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
