// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Death.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

UWxAbility_Death::UWxAbility_Death()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Death);
	SetAssetTags(AssetTags);

	// 사망하면 진행 중이던 액션(적 패턴 포함)을 끊고 이후 액션도 막는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

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
	
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Avatar && Avatar->GetMesh())
	{
		// 공격 채널 응답만 끈다.
		// CollisionEnabled를 내리면 ShouldCreatePhysicsState가 false가 되어 피직스 바디가 통째로 파괴되고, 래그돌 진입에서 다시 만드는 왕복이 생긴다.
		Avatar->GetMesh()->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Ignore);
	}
	
	AAIController* AIController = Avatar ? Cast<AAIController>(Avatar->GetController()) : nullptr;
	if (UBrainComponent* Brain = AIController ? AIController->GetBrainComponent() : nullptr)
	{
		Brain->StopLogic(TEXT("Death"));
	}

	PlayDeathMontageOrRagdoll();
}

void UWxAbility_Death::HandleMontageCompleted()
{
	// 의도한 사망 포즈로 끝났으므로 래그돌로 넘기지 않는다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Death::HandleMontageInterrupted()
{
	// 외부가 사망 몽타주를 끊은 비정상 경로 — 래그돌로 폴백한다.
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
		// 활성 HitReact 몽타주가 BlendOut될 시간을 주고 래그돌로 인계한다.
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
	// 오너 클라 인스턴스도 이 경로에 들어오므로, 루스 태그가 중복 추가되지 않게 서버에서만 발행한다.
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Ragdoll, 1, EGameplayTagReplicationState::TagOnly);
	}
}
