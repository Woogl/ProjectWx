// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
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
	// 콤보는 EndAbility 후 재발동 방식이라 자기 차단이 다음 단계를 막지 않으며, 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	// 활성 중 자기 재진입은 입력 라우팅(활성 스펙은 InputPressed로 전달)과 엔진의 InstancedPerActor 중복 발동 거부가 막으므로 별도 태그 차단이 필요 없다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 콤보 재발동이면 저장된 경로, 아니면 입력 종류로 첫 경로 결정
	FString ActivationPath;
	if (!NextComboPath.IsEmpty() && ComboMap.Contains(FName(*NextComboPath)))
	{
		ActivationPath = NextComboPath;
	}
	else
	{
		const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
		ActivationPath = (ASC && ASC->GetLastPressedInputTag() == WxGameplayTags::Input_Attack_Heavy)
			? TEXT("H")
			: TEXT("L");
	}
	NextComboPath.Empty();

	if (!ComboMap.Contains(FName(*ActivationPath)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentPath = ActivationPath;
	PlayComboMontage();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CurrentPath.Empty();
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

	WaitForComboInput();
}

void UWxAbility_Attack::WaitForComboInput()
{
	// 터미널 노드에서도 첫타 재시작 입력을 받아야 하므로 항상 입력을 대기한다.
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputTask->OnPress.AddDynamic(this, &UWxAbility_Attack::HandleComboInputPressed);
	WaitInputTask->ReadyForActivation();
}

bool UWxAbility_Attack::HasNextCombo() const
{
	return ComboMap.Contains(FName(*(CurrentPath + TEXT("L")))) || ComboMap.Contains(FName(*(CurrentPath + TEXT("H"))));
}

void UWxAbility_Attack::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Attack::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Attack::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Attack::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Attack::HandleComboInputPressed(float TimeWaited)
{
	UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (!ASC)
	{
		return;
	}

	// ComboWindow 구간이 아니면 콤보 입력을 받지 않는다.
	if (!ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		WaitForComboInput();
		return;
	}

	const TCHAR* Suffix = (ASC->GetLastPressedInputTag() == WxGameplayTags::Input_Attack_Heavy)
		? TEXT("H")
		: TEXT("L");

	// 다음 경로가 있으면 콤보를 진행한다.
	if (ComboMap.Contains(FName(*(CurrentPath + Suffix))))
	{
		Reactivate(CurrentPath + Suffix);
		return;
	}

	// 현재 노드에 이어갈 경로가 없으면(터미널) 첫타로 재시작한다.
	if (!HasNextCombo() && ComboMap.Contains(FName(Suffix)))
	{
		Reactivate(Suffix);
		return;
	}

	// 해당 입력으로 진행/재시작할 수 없으면 다음 입력을 다시 대기한다.
	WaitForComboInput();
}

void UWxAbility_Attack::Reactivate(const FString& Path)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// SpecHandle을 EndAbility 전에 캡처. NextComboPath는 재발동 시 ActivateAbility가 소비한다.
	const FGameplayAbilitySpecHandle SpecHandle = CurrentSpecHandle;
	NextComboPath = Path;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

	if (!ASC->TryActivateAbility(SpecHandle))
	{
		// 재발동 실패(쿨다운, 비용 부족, 차단 등) — 다음 신규 발동이 NextComboPath를 잘못 쓰지 않도록 클리어
		NextComboPath.Empty();
	}
}
