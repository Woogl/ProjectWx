// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	ActivationInputTag = WxGameplayTags::Input_Attack;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Attack);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Attack);
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

	const TCHAR* Suffix = (ASC->GetLastPressedInputTag() == WxGameplayTags::Input_Attack_Heavy)
		? TEXT("H")
		: TEXT("L");

	// ANS_ComboWindow 구간이면 다음 경로로 콤보를 진행한다.
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow) && ComboMap.Contains(FName(*(CurrentPath + Suffix))))
	{
		Reactivate(CurrentPath + Suffix);
		return;
	}

	// ANS_CancelWindow 구간이면 후딜을 끊고 첫타로 재시작한다.
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_CancelWindow) && ComboMap.Contains(FName(Suffix)))
	{
		Reactivate(Suffix);
		return;
	}

	// 어느 윈도우에도 해당하지 않으면 다음 입력을 다시 대기한다.
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
