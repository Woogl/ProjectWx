// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Ultimate.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_SuperArmor.h"
#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "LevelSequence.h"
#include "WxGameplayTags.h"
#include "Cutscene/WxSkillCutsceneComponent.h"

UWxAbility_Ultimate::UWxAbility_Ultimate()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Ultimate);
	SetAssetTags(AssetTags);
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Ultimate);
	ActivationOwnedEffects.Add(UWxEffect_SuperArmor::StaticClass());

	CooldownGameplayEffectClass = UWxEffect_Cooldown_Ultimate::StaticClass();
}

void UWxAbility_Ultimate::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (!CutsceneSequence.IsNull() && !CutsceneSequence.Get())
	{
		CutscenePreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(CutsceneSequence.ToSoftObjectPath());
	}
}

bool UWxAbility_Ultimate::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UWxSkillCutsceneComponent* Coordinator = UWxSkillCutsceneComponent::Get(GetWorld());
	if (Coordinator && Coordinator->IsBusy())
	{
		return false;
	}
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWxAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 부여 직후 곧바로 발동하면 프리로드가 아직 도착하지 않았을 수 있다.
	ULevelSequence* Sequence = CutsceneSequence.Get();
	if (!Sequence)
	{
		Sequence = CutsceneSequence.LoadSynchronous();
	}

	UWxSkillCutsceneComponent* Coordinator = nullptr;
	if (Sequence && ActorInfo->IsNetAuthority())
	{
		Coordinator = UWxSkillCutsceneComponent::Get(GetWorld());
		if (!Coordinator || !Coordinator->Reserve(this))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	// 서버는 예약 후 비용을 확정한다. 거절된 로컬 발동의 예측 비용은 GAS가 롤백한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if (Coordinator)
		{
			Coordinator->Cancel(this);
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Sequence)
	{
		bLocalPresentationPending = !ActorInfo->IsNetAuthority() && ActorInfo->IsLocallyControlled();
		UWxAbilityTask_PlaySkillCutscene* CutsceneTask = UWxAbilityTask_PlaySkillCutscene::CreateTask(this, Sequence, 0.001f);
		CutsceneTask->OnCompleted.AddDynamic(this, &UWxAbility_Ultimate::HandleCutsceneCompleted);
		CutsceneTask->OnCancelled.AddDynamic(this, &UWxAbility_Ultimate::HandleCutsceneCancelled);
		CutsceneTask->ReadyForActivation();
		return;
	}

	HandleCutsceneCompleted();
}

void UWxAbility_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 서버의 몽타주가 먼저 끝나도 로컬 컷신/몽타주는 자신의 완료 콜백까지 살아 있어야 한다.
	// RemoteEndOrCancelAbility는 이 호출 전에 RemoteInstanceEnded를 설정한다. 강제 취소는 보류하지 않는다.
	if (bLocalPresentationPending && ActorInfo && !ActorInfo->IsNetAuthority()
		&& RemoteInstanceEnded && !bReplicateEndAbility && !bWasCancelled)
	{
		return;
	}
	bLocalPresentationPending = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Ultimate::HandleCutsceneCompleted()
{
	// 몽타주 없이 컷신만으로 끝나는 구성도 정상이다.
	if (!UltimateMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!PlayMontage(UltimateMontage))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Ultimate::HandleCutsceneCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

