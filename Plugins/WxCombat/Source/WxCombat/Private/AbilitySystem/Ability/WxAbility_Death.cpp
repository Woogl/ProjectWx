// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Death.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

UWxAbility_Death::UWxAbility_Death()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
	// 사망은 Attack/Skill이 건 BlockAbilitiesWithTag(Ability)에 절대 막히면 안 되는 반응 어빌리티다.
	// 차단은 상대 AssetTag가 Ability.*에 매칭될 때만 걸리므로, 매칭될 태그가 없도록 AssetTag를 비워 둔다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::State_Dead;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AIController = Avatar ? Cast<AAIController>(Avatar->GetController()) : nullptr;
	if (UBrainComponent* Brain = AIController ? AIController->GetBrainComponent() : nullptr)
	{
		Brain->StopLogic(TEXT("Death"));
	}

	PlayDeathMontageOrRagdoll();
}

void UWxAbility_Death::HandleMontageCompleted()
{
	// 사망 몽타주가 의도한 포즈로 자연 종료 — 래그돌 없이 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Death::HandleMontageInterrupted()
{
	// 외부 시스템이 사망 몽타주를 끊은 비정상 경로 — 래그돌로 안전 폴백
	RagdollAndEnd(true);
}

void UWxAbility_Death::HandleMontageCancelled()
{
	RagdollAndEnd(true);
}

void UWxAbility_Death::PlayDeathMontageOrRagdoll()
{
	if (!DeathMontage)
	{
		// 활성 HitReact 몽타주가 BlendOut될 시간을 주고 래그돌로 인계
		UWorld* World = GetWorld();
		if (!World)
		{
			RagdollAndEnd(false);
			return;
		}
		constexpr float DelayTime = 0.15f;
		World->GetTimerManager().SetTimer(RagdollDelayTimerHandle, this, &UWxAbility_Death::HandleRagdollDelayElapsed, DelayTime, false);
		return;
	}

	// HitReact 등 활성 몽타주는 PlayMontageAndWait가 BlendOut으로 인계받는다.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		RagdollAndEnd(true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Death::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Death::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Death::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Death::HandleRagdollDelayElapsed()
{
	RagdollAndEnd(false);
}

void UWxAbility_Death::RagdollAndEnd(bool bWasCancelled)
{
	EnableRagdoll();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UWxAbility_Death::EnableRagdoll()
{
	UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (WxASC)
	{
		WxASC->SetRagdollActive(true);
	}
}
