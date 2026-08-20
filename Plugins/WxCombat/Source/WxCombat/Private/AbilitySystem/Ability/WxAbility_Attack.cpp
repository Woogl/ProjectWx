// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack);
	AssetTags.AddTag(WxGameplayTags::Trait_Ability_Exclusive);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Attack);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 즉시 회피·가드로 빠져나가는 것을 막아 공격에 리스크를 부여한다.
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이가 연다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Trait_Ability_Exclusive);

	bRetriggerInstancedAbility = true;
}

bool UWxAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	// 콤보 진행. 자기 차단은 곧 EndAbility가 푸니 무시한다.
	if (Spec && Spec->IsActive())
	{
		return ASC
			&& ASC->HasMatchingGameplayTag(WxGameplayTags::State_ComboWindow)
			&& !ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags)
			&& CheckCooldown(Handle, ActorInfo, OptionalRelevantTags)
			&& CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	// 끊고 들어가는 발동. 취소 대상이 건 차단을 넘긴다.
	if (ASC && HasActiveCancelTarget(*ASC))
	{
		return !ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags)
			&& CheckCooldown(Handle, ActorInfo, OptionalRelevantTags)
			&& CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 터미널 단에서는 첫 단으로 되감긴다.
	ComboIndex = ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;

	UAnimMontage* ComboMontage = ComboMontages.IsValidIndex(ComboIndex) ? ComboMontages[ComboIndex].Get() : nullptr;
	if (!PlayMontage(ComboMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 캔슬 종료만 되돌린다 — 콤보 재발동도 이 종료를 지나가는데, 그쪽은 bWasCancelled=false라 진행 단이 보존된다.
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Attack::HandleMontageCompleted()
{
	// 캔슬이 아니라 EndAbility가 되돌려주지 않는다.
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}

bool UWxAbility_Attack::HasActiveCancelTarget(const UAbilitySystemComponent& ASC) const
{
	if (CancelAbilitiesWithTag.IsEmpty())
	{
		return false;
	}

	// 자기 스펙이 활성이면 호출자가 콤보 재발동으로 먼저 처리하므로, 여기 걸리는 건 전부 남의 것이다.
	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasAny(CancelAbilitiesWithTag))
		{
			return true;
		}
	}

	return false;
}
