// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Pattern_Phase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "WxGameplayTags.h"

UWxAbility_Pattern_Phase::UWxAbility_Pattern_Phase()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Pattern_Phase);
	SetAssetTags(AssetTags);

	// 전환 중에는 다른 모든 어빌리티를 취소/차단하고, 패턴 발동 중임을 알려 일반 피격 반응을 억제한다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Pattern_Phase);
	ActivationOwnedTags.AddTag(WxGameplayTags::State_SuperArmor);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

#if WITH_EDITOR
bool UWxAbility_Pattern_Phase::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	if (InProperty)
	{
		const FName PropertyName = InProperty->GetFName();

		// AI가 직접 발동하므로 입력 태그는 사용하지 않는다.
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, ActivationInputTag))
		{
			return false;
		}
	}

	return true;
}
#endif

void UWxAbility_Pattern_Phase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 발동 시작 시 페이즈를 올린다.
	AdvancePhase();

	// 컷신 에셋이 없으면 바로 몽타주 단계로 진행
	HandleCutsceneCompleted();
}

void UWxAbility_Pattern_Phase::AdvancePhase()
{
	// WxCombat은 WxAI를 참조할 수 없어 Blackboard "Phase" 키 이름을 직접 사용한다.
	// (WxAI/WxBlackboardKeys::Phase 와 동일한 이름이어야 한다.)
	static const FName PhaseBlackboardKeyName = TEXT("Phase");

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AIController = AvatarPawn ? Cast<AAIController>(AvatarPawn->GetController()) : nullptr;
	UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
	if (!Blackboard)
	{
		return;
	}

	Blackboard->SetValueAsInt(PhaseBlackboardKeyName, NextPhase);
}

void UWxAbility_Pattern_Phase::HandleCutsceneCompleted()
{
	// 몽타주가 없으면 전환 완료: 종료한다.
	if (!TransitionMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, TransitionMontage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Pattern_Phase::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Pattern_Phase::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Pattern_Phase::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Pattern_Phase::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Pattern_Phase::HandleCutsceneCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Pattern_Phase::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Pattern_Phase::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Pattern_Phase::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Pattern_Phase::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
