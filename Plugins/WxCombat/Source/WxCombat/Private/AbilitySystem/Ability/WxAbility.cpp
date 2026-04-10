// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility.h"
#include "AbilitySystem/Effect/WxEffect_CooldownBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UWxAbility::UWxAbility()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UWxAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : OnActivateEffects)
	{
		if (EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			}
		}
	}
}

void UWxAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

bool UWxAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 활성 쿨다운 GE의 스택 합계가 StackLimitCount 미만이면 사용 가능 (StackLimitCount=1이면 GAS 기본 동작과 동일).
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = CooldownGE->GetClass();

	int32 ConsumedCharges = 0;
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& ActiveHandle : Handles)
	{
		if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveHandle))
		{
			ConsumedCharges += ActiveGE->Spec.GetStackCount();
		}
	}

	const int32 MaxCharges = FMath::Max(1, CooldownGE->StackLimitCount);
	if (ConsumedCharges < MaxCharges)
	{
		return true;
	}

	// 사용 불가: GAS 표준대로 OptionalRelevantTags에 쿨다운 태그를 채워준다.
	if (OptionalRelevantTags)
	{
		if (const FGameplayTagContainer* CooldownTags = GetCooldownTags())
		{
			OptionalRelevantTags->AppendTags(*CooldownTags);
		}
	}
	return false;
}
