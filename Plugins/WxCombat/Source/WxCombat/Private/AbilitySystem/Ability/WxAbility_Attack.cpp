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

	// 공격은 시동·타격 중 다른 GA로 캔슬되지 않는다(커밋).
	// 즉시 회피·가드로 빠져나가는 것을 막아 공격에 리스크를 부여하려는 의도다.
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	// 콤보는 재발동으로 다음 단계로 넘어간다. 콤보 윈도우 판정은 CanActivateAbility가 담당한다.
	bRetriggerInstancedAbility = true;
}

bool UWxAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	// 활성 중 재발동 = 콤보 진행. 자기 차단(Ability.Exclusive)은 곧 EndAbility가 해제하므로 무시하고 콤보 윈도우 안에서만 연다.
	if (Spec && Spec->IsActive())
	{
		return ASC
			&& ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow)
			&& !ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags)
			&& CheckCooldown(Handle, ActorInfo, OptionalRelevantTags)
			&& CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	// 끊고 들어가는 발동. 취소 대상이 돌고 있으면 그것이 건 차단은 넘긴다 — 엔진이 차단을 캔슬보다 먼저 보므로, 이 경로가 없으면 취소가 실행될 기회 자체가 없다.
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

	// 세트 인덱스가 유효하면 재발동으로 이어온 콤보라 그 세트를 유지하고, 아니면 신규 발동이라 태그로 새로 고른다.
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

	// 터미널 인덱스에서 재발동하면 첫 인덱스로 되감아 재시작한다.
	const TArray<TObjectPtr<UAnimMontage>>& ComboMontages = MontageSets[CurrentSetIndex].ComboMontages;
	CurrentMontageIndex = ComboMontages.IsValidIndex(CurrentMontageIndex + 1) ? CurrentMontageIndex + 1 : 0;

	PlayCurrentMontage();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 콤보 재발동 시 엔진이 이 EndAbility를 먼저 호출한다.
	// 몽타주 태스크를 콜백 해제(EndTask) 후 정리해, 그 종료가 Interrupted/Cancelled 핸들러를 깨워 진행 상태를 되돌리는 것을 막는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	// 외부 캔슬에서는 위 EndTask가 몽타주 핸들러를 끊어 진행 상태를 되돌릴 핸들러가 실행되지 않으므로 여기서 직접 리셋한다.
	// 콤보 재발동은 bWasCancelled=false로 들어오므로 진행 중 상태는 보존된다.
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

	// 자기 스펙이 활성인 경우는 호출자가 콤보 재발동으로 먼저 처리하므로, 여기 걸리는 활성 스펙은 전부 남의 것이다.
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

	// 배열 순서가 곧 우선순위다. 요구사항이 비면 RequirementsMet가 항상 참이라 그 세트가 조건 없이 걸린다.
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
	// 콤보 미입력으로 자연 종료 → 콤보 리셋(다음 발동은 세트 선택부터).
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
