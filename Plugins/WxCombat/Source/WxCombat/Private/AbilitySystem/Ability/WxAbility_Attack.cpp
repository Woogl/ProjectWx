// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 즉시 회피·가드로 빠져나가는 것을 막아 공격에 리스크를 부여한다.
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이가 연다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

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
			&& ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow)
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

	// 세트 인덱스가 살아 있으면 재발동으로 이어온 콤보다.
	if (!MontageSets.IsValidIndex(CurrentSetIndex))
	{
		CurrentSetIndex = ResolveMontageSetIndex();
		CurrentMontageIndex = INDEX_NONE;
	}

	if (!MontageSets.IsValidIndex(CurrentSetIndex))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const TArray<TObjectPtr<UAnimMontage>>& ComboMontages = MontageSets[CurrentSetIndex].ComboMontages;
	CurrentMontageIndex = ComboMontages.IsValidIndex(CurrentMontageIndex + 1) ? CurrentMontageIndex + 1 : 0;

	PlayCurrentMontage();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 콤보 재발동 시 엔진이 이 EndAbility를 먼저 부른다 — EndTask로 콜백을 끊지 않으면 그 종료가 Interrupted 핸들러를 깨워 진행 상태를 되돌린다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	// 외부 캔슬은 위 EndTask 탓에 핸들러가 돌지 않으므로 여기서 직접 리셋한다.
	// 콤보 재발동은 bWasCancelled=false라 상태가 보존된다.
	if (bWasCancelled)
	{
		CurrentSetIndex = INDEX_NONE;
		CurrentMontageIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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

int32 UWxAbility_Attack::ResolveMontageSetIndex() const
{
	FGameplayTagContainer OwnedTags;
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	for (int32 Index = 0; Index < MontageSets.Num(); ++Index)
	{
		if (MontageSets[Index].EntryTagRequirements.RequirementsMet(OwnedTags))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UWxAbility_Attack::PlayCurrentMontage()
{
	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	const TArray<TObjectPtr<UAnimMontage>>& ComboMontages = MontageSets[CurrentSetIndex].ComboMontages;
	UAnimMontage* Montage = ComboMontages.IsValidIndex(CurrentMontageIndex) ? ComboMontages[CurrentMontageIndex].Get() : nullptr;
	if (!Montage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Attack::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Attack::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Attack::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Attack::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Attack::HandleMontageCompleted()
{
	CurrentSetIndex = INDEX_NONE;
	CurrentMontageIndex = INDEX_NONE;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Attack::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Attack::HandleMontageInterrupted()
{
	CurrentSetIndex = INDEX_NONE;
	CurrentMontageIndex = INDEX_NONE;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Attack::HandleMontageCancelled()
{
	CurrentSetIndex = INDEX_NONE;
	CurrentMontageIndex = INDEX_NONE;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
