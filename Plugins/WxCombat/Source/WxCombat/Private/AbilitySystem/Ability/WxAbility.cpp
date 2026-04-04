// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystemComponent.h"

UWxAbility::UWxAbility()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UWxAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (MaxCharges > 1 && ChargeTag.IsValid() && CooldownTag.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC && ASC->IsOwnerActorAuthoritative())
		{
			SetChargeCount(ASC, MaxCharges);

			ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UWxAbility::HandleCooldownTagChanged);
		}
	}

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UWxAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (MaxCharges > 1 && ChargeTag.IsValid() && CooldownTag.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC && ASC->IsOwnerActorAuthoritative())
		{
			ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);

			SetChargeCount(ASC, 0);
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

UGameplayEffect* UWxAbility::GetCooldownGameplayEffect() const
{
	// Super(UGameplayAbility)에 CooldownGameplayEffectClass가 설정된 경우 그대로 사용
	if (UGameplayEffect* ParentCooldownGE = Super::GetCooldownGameplayEffect())
	{
		return ParentCooldownGE;
	}

	// ApplyCooldown에서 동적 Duration으로 직접 적용하므로 여기서는 nullptr 반환.
	// CDO를 반환하면 Super::ApplyCooldown이 기본 Duration(1초)으로 이중 적용하게 된다.
	return nullptr;
}

const FGameplayTagContainer* UWxAbility::GetCooldownTags() const
{
	CooldownTagContainer.Reset();

	const FGameplayTagContainer* ParentTags = Super::GetCooldownTags();
	if (ParentTags)
	{
		CooldownTagContainer.AppendTags(*ParentTags);
	}

	if (CooldownTag.IsValid())
	{
		CooldownTagContainer.AddTag(CooldownTag);
	}

	return &CooldownTagContainer;
}

bool UWxAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (MaxCharges > 1 && ChargeTag.IsValid())
	{
		const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC && ASC->GetTagCount(ChargeTag) > 0)
		{
			return true;
		}
		return false;
	}

	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UWxAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.f || !CooldownTag.IsValid())
	{
		return;
	}

	if (MaxCharges > 1 && ChargeTag.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (!ASC)
		{
			return;
		}

		// 충전 소모
		const int32 CurrentCount = ASC->GetTagCount(ChargeTag);
		SetChargeCount(ASC, FMath::Max(CurrentCount - 1, 0));

		// 쿨다운이 아직 진행 중이 아닐 때만 재충전 타이머 시작
		if (!ASC->HasMatchingGameplayTag(CooldownTag))
		{
			const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetDuration(CooldownDuration, true);
				SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			}
		}
		return;
	}

	// 기본 쿨다운
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetDuration(CooldownDuration, true);
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

void UWxAbility::HandleCooldownTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		return;
	}

	// 쿨다운 만료: 충전 1회 재충전
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	const int32 CurrentCount = ASC->GetTagCount(ChargeTag);
	const int32 NewChargeCount = FMath::Min(CurrentCount + 1, MaxCharges);
	SetChargeCount(ASC, NewChargeCount);

	// 아직 최대 충전이 아니면 다음 재충전을 위해 쿨다운 재적용
	if (NewChargeCount < MaxCharges)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel(), EffectContext);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetDuration(CooldownDuration, true);
			SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void UWxAbility::SetChargeCount(UAbilitySystemComponent* ASC, int32 NewCount) const
{
	if (!ASC || !ChargeTag.IsValid())
	{
		return;
	}

	const int32 CurrentCount = ASC->GetTagCount(ChargeTag);
	if (CurrentCount > NewCount)
	{
		ASC->RemoveLooseGameplayTag(ChargeTag, CurrentCount - NewCount, EGameplayTagReplicationState::TagAndCountToAll);
	}
	else if (NewCount > CurrentCount)
	{
		ASC->AddLooseGameplayTag(ChargeTag, NewCount - CurrentCount, EGameplayTagReplicationState::TagAndCountToAll);
	}
}
