// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// BP에서 WxGameplayTags::Ability_Skill_1~4로 설정한다.
	//AssetTags.AddTag(WxGameplayTags::Ability_Skill_@);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 스킬은 재생 중 다른 GA로 캔슬되지 않는다. (PC규격서 §5.6)
	// 콤보는 EndAbility 후 재발동 방식이라 자기 차단이 다음 단계를 막지 않으며, 후딜 캔슬은 몽타주 StartRecovery 노티파이로 허용한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	// BP에서 WxGameplayTags::Input_Skill_~4로 설정한다.
	//ActivationInputTag = WxGameplayTags::Input_Skill_@;
}

void UWxAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SkillMontages.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 콤보 재발동으로 들어왔다면 저장된 인덱스, 아니면 0부터 시작
	const int32 ActivationIndex = (NextComboIndex != INDEX_NONE && SkillMontages.IsValidIndex(NextComboIndex))
		? NextComboIndex
		: 0;
	NextComboIndex = INDEX_NONE;

	CurrentIndex = ActivationIndex;
	PlayCurrentMontage();
}

void UWxAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CurrentIndex = 0;
}

void UWxAbility_Skill::PlayCurrentMontage()
{
	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UAnimMontage* Montage = SkillMontages.IsValidIndex(CurrentIndex) ? SkillMontages[CurrentIndex].Get() : nullptr;
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

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Skill::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Skill::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Skill::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Skill::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	WaitForComboInput();
}

void UWxAbility_Skill::WaitForComboInput()
{
	// 터미널 인덱스에서도 첫 인덱스 재시작 입력을 받아야 하므로 항상 입력을 대기한다.
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputTask->OnPress.AddDynamic(this, &UWxAbility_Skill::HandleComboInputPressed);
	WaitInputTask->ReadyForActivation();
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Skill::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Skill::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Skill::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Skill::HandleComboInputPressed(float TimeWaited)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		// 콤보 윈도우 밖이면 다음 입력을 다시 대기
		WaitForComboInput();
		return;
	}

	// 다음 인덱스가 있으면 콤보를 진행하고, 없으면(터미널) 첫 인덱스로 재시작한다.
	const int32 NextIndex = SkillMontages.IsValidIndex(CurrentIndex + 1) ? (CurrentIndex + 1) : 0;

	// 현재 활성화를 종료하고 동일 spec을 즉시 재발동. NextComboIndex는 다음 ActivateAbility가 소비.
	const FGameplayAbilitySpecHandle SpecHandle = CurrentSpecHandle;
	NextComboIndex = NextIndex;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

	if (!ASC->TryActivateAbility(SpecHandle))
	{
		// 재발동 실패 (쿨다운, 비용 부족, 차단 등) — 다음 신규 발동이 NextComboIndex를 잘못 쓰지 않도록 클리어
		NextComboIndex = INDEX_NONE;
	}
}
