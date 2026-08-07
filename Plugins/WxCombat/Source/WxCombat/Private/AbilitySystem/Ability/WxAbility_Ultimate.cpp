// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Ultimate.h"
#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "LevelSequence.h"
#include "WxGameplayTags.h"

UWxAbility_Ultimate::UWxAbility_Ultimate()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Ultimate);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);

	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
	ActivationOwnedTags.AddTag(WxGameplayTags::State_SuperArmor);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Groggy);
}

void UWxAbility_Ultimate::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// 컷신을 부여 시점에 미리 잡아 둔다. 발동은 연출이 시작되는 지점이라 그때의 콜드 로드가 그대로 히치로 보인다.
	if (!CutsceneSequence.IsNull() && !CutsceneSequence.Get())
	{
		CutscenePreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(CutsceneSequence.ToSoftObjectPath());
	}
}

void UWxAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 컷신 재생. 부여 시점 프리로드가 끝나 있으면 그 포인터를 그대로 쓰고,
	// 부여 직후 곧바로 발동한 경우처럼 아직 도착하지 않았으면 동기 로드로 폴백한다(동작 후퇴 없음).
	ULevelSequence* Sequence = CutsceneSequence.Get();
	if (!Sequence)
	{
		Sequence = CutsceneSequence.LoadSynchronous();
	}

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

