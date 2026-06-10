// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Ultimate.h"
#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "LevelSequence.h"
#include "WxGameplayTags.h"

UWxAbility_Ultimate::UWxAbility_Ultimate()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Ultimate);
	SetAssetTags(AssetTags);

	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationOwnedTags.AddTag(WxGameplayTags::State_SuperArmor);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Groggy);
}

void UWxAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 컷신 재생
	ULevelSequence* Sequence = CutsceneSequence.LoadSynchronous();
	if (Sequence)
	{
		UWxAbilityTask_PlaySkillCutscene* CutsceneTask = UWxAbilityTask_PlaySkillCutscene::CreateTask(this, Sequence, 0.001f);
		if (CutsceneTask)
		{
			CutsceneTask->OnCompleted.AddDynamic(this, &UWxAbility_Ultimate::HandleCutsceneCompleted);
			CutsceneTask->OnCancelled.AddDynamic(this, &UWxAbility_Ultimate::HandleCutsceneCancelled);
			CutsceneTask->ReadyForActivation();
			return;
		}
	}

	// 컷신 에셋이 없으면 바로 몽타주 단계로 진행
	HandleCutsceneCompleted();
}

void UWxAbility_Ultimate::HandleCutsceneCompleted()
{
	if (!UltimateMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, UltimateMontage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Ultimate::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Ultimate::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Ultimate::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Ultimate::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Ultimate::HandleCutsceneCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Ultimate::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Ultimate::HandleMontageBlendOut()
{
}

void UWxAbility_Ultimate::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Ultimate::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

