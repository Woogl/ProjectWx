// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Ultimate.h"
#include "AbilitySystem/Effect/WxEffect_SuperArmor.h"
#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "AnimNotify/WxAnimNotify_SkillCutscene.h"
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

	ULevelSequence* Sequence = nullptr;
	if (const UAnimMontage* UltimateMontage = GetMontage())
	{
		for (const FAnimNotifyEvent& NotifyEvent : UltimateMontage->Notifies)
		{
			if (const UWxAnimNotify_SkillCutscene* CutsceneNotify = Cast<UWxAnimNotify_SkillCutscene>(NotifyEvent.Notify))
			{
				Sequence = CutsceneNotify->Sequence;
				break;
			}
		}
	}

	UWxSkillCutsceneComponent* Coordinator = nullptr;
	if (Sequence && ActorInfo->IsNetAuthority())
	{
		// 비용 확정보다 먼저 컷신을 연다. 뒤로 밀면 컷신을 못 열고 쿨다운만 쓰는 창이 생긴다.
		// 거절된 로컬 발동의 예측 비용은 GAS가 롤백한다.
		Coordinator = UWxSkillCutsceneComponent::Get(GetWorld());
		if (!Coordinator || !Coordinator->Start(this, Sequence, 0.001f))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

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
		UWxAbilityTask_PlaySkillCutscene* CutsceneTask = UWxAbilityTask_PlaySkillCutscene::CreateTask(this);
		CutsceneTask->OnCompleted.AddDynamic(this, &UWxAbility_Ultimate::HandleCutsceneCompleted);
		CutsceneTask->OnCancelled.AddDynamic(this, &UWxAbility_Ultimate::HandleCutsceneCancelled);
		CutsceneTask->ReadyForActivation();
		return;
	}

	HandleCutsceneCompleted();
}

void UWxAbility_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 로컬 몽타주를 재생하는 동안에는 서버의 정상 종료를 보류하고, 몽타주 완료 콜백이 끝낸다.
	// 컷신을 기다리는 동안에는 보류하지 않는다 — 이 머신이 놓친 세션은 종료 통지가 오지 않아 이 종료만이 어빌리티를 닫는다.
	// RemoteEndOrCancelAbility는 이 호출 전에 RemoteInstanceEnded를 설정한다. 강제 취소는 보류하지 않는다.
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && !ActorInfo->IsNetAuthority() && ASC->IsAnimatingAbility(this)
		&& RemoteInstanceEnded && !bReplicateEndAbility && !bWasCancelled)
	{
		return;
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Ultimate::HandleCutsceneCompleted()
{
	if (!PlayMontage(GetMontage()))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Ultimate::HandleCutsceneCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

