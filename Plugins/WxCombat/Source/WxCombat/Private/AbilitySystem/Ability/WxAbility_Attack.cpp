// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 공격은 시동·타격 중 다른 GA로 캔슬되지 않는다(커밋).
	// 즉시 회피·가드로 빠져나가는 것을 막아 공격에 리스크를 부여하려는 의도다.
	// 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	// 콤보는 재발동으로 다음 단계로 넘어간다. 콤보 윈도우·분기 유효성 판정은 CanActivateAbility가 담당한다.
	bRetriggerInstancedAbility = true;
}

bool UWxAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	// 활성 중 재발동 = 콤보 진행. 이 어빌리티가 건 자기 차단(Ability)은 곧 EndAbility가 해제하므로 무시하고,
	// 콤보 윈도우 안에서 눌린 입력이 현재 노드를 이을 수 있을 때만 허용한다(잇지 못하면 거부해 현재 몽타주를 유지·무시).
	if (Spec && Spec->IsActive())
	{
		if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
		{
			return false;
		}
		if (ResolveNextComboPath().IsEmpty())
		{
			return false;
		}
		if (ASC->HasAnyMatchingGameplayTags(ActivationBlockedTags))
		{
			return false;
		}
		return CheckCooldown(Handle, ActorInfo, OptionalRelevantTags) && CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

bool UWxAbility_Attack::IsActivationInput(const UInputAction* Action) const
{
	return Super::IsActivationInput(Action) || (Action && Action == HeavyInputAction);
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartHitStopListener();

	// 재발동으로 이어온 콤보면 다음 경로, 신규 발동이면 입력 종류로 첫타.
	// CurrentPath는 재발동 사이 보존되고(그래서 여기서 이어감), 콤보 자연 종료 시 몽타주 핸들러가 비운다.
	const FString NextPath = ResolveNextComboPath();
	if (NextPath.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentPath = NextPath;
	PlayComboMontage();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 콤보 재발동 시 엔진이 이 EndAbility를 먼저 호출한다.
	// 몽타주 태스크를 콜백 해제(EndTask) 후 정리해, 그 종료가 Interrupted/Cancelled 핸들러를 깨워 CurrentPath를 되돌리는 것을 막는다.
	// CurrentPath는 여기서 비우지 않는다(재발동 시 보존). 콤보 자연 종료는 몽타주 핸들러가 비운다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Attack::PlayComboMontage()
{
	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UAnimMontage* Montage = ComboMap.FindRef(FName(*CurrentPath));
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

bool UWxAbility_Attack::HasNextCombo() const
{
	return ComboMap.Contains(FName(*(CurrentPath + TEXT("L")))) || ComboMap.Contains(FName(*(CurrentPath + TEXT("H"))));
}

FString UWxAbility_Attack::ResolveNextComboPath() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	const TCHAR* Suffix = (ASC && ASC->GetLastPressedInputAction() == HeavyInputAction)
		? TEXT("H")
		: TEXT("L");

	// 신규 발동: 입력 종류로 첫타 결정.
	if (CurrentPath.IsEmpty())
	{
		return ComboMap.Contains(FName(Suffix)) ? FString(Suffix) : FString();
	}

	// 콤보 진행: 현재 경로에 입력을 이어붙일 수 있으면 진행.
	const FString ExtendedPath = CurrentPath + Suffix;
	if (ComboMap.Contains(FName(*ExtendedPath)))
	{
		return ExtendedPath;
	}

	// 현재 노드에 이어갈 경로가 없으면(터미널) 첫타로 재시작.
	if (!HasNextCombo() && ComboMap.Contains(FName(Suffix)))
	{
		return FString(Suffix);
	}

	// 이 입력으로는 진행/재시작할 수 없으면 무시(빈 문자열).
	return FString();
}

void UWxAbility_Attack::HandleMontageCompleted()
{
	// 콤보 미입력으로 자연 종료 → 콤보 리셋(다음 발동은 첫타부터).
	CurrentPath.Empty();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Attack::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Attack::HandleMontageInterrupted()
{
	CurrentPath.Empty();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Attack::HandleMontageCancelled()
{
	CurrentPath.Empty();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
