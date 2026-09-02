// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_DrainGP.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

UWxAbility_Groggy::UWxAbility_Groggy()
{
	// 로컬 조종 액터에는 복제 몽타주가 적용되지 않아 소유 클라도 활성화해야 한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Groggy);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Groggy);
	
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	ActivationGroup = EWxAbilityActivationGroup::Override;

	// Override 어빌리티는 캔슬되지 않아, 처형 짝 피격처럼 겹쳐야 할 반응이 보존된다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Groggy;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GroggyMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartMontagePolling();

	// 클라의 복제 GP로 종료를 판정하면 서버와 종료 시점이 어긋난다.
	if (ActorInfo->IsNetAuthority())
	{
		StartGroggyDrain(Handle, ActorInfo, ActivationInfo);
	}

	SetAILogicPaused(ActorInfo, true);

	HandleMontagePollTick();
}

void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	StopMontagePolling();

	if (ActorInfo)
	{
		SetAILogicPaused(ActorInfo, false);

		if (ActorInfo->AbilitySystemComponent.IsValid())
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

			// GroggyMontage 미설정 경로에서는 즉시 종료될 수 있다.
			if (GroggyMontage)
			{
				ASC->StopMontageIfCurrent(*GroggyMontage);
			}

			StopGroggyDrain(*ASC);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Groggy::HandleGPChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::HandleMontagePollTick()
{
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
	{
		// 사망 어빌리티가 Override 그로기를 취소하지 못해 여기서 종료한다.
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (ASC->GetCurrentMontage() != nullptr)
	{
		return;
	}

	ASC->PlayMontage(this, CurrentActivationInfo, GroggyMontage, 1.f);
}

void UWxAbility_Groggy::StartMontagePolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MontagePollingTimerHandle, this, &UWxAbility_Groggy::HandleMontagePollTick, 0.1f, true);
	}
}

void UWxAbility_Groggy::StopMontagePolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MontagePollingTimerHandle);
	}
}

void UWxAbility_Groggy::StartGroggyDrain(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	GPDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute())
		.AddUObject(this, &UWxAbility_Groggy::HandleGPChanged);

	const float GroggyDuration = GroggyMontage->GetPlayLength();
	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainGP::StaticClass(), GetAbilityLevel());
	if (DrainSpecHandle.IsValid())
	{
		// 잠가서 넣어야 적용 시점의 Def 기반 Duration 재계산이 이 값을 덮어쓰지 않는다.
		DrainSpecHandle.Data->SetDuration(GroggyDuration, true);
		DrainGPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
	}
}

void UWxAbility_Groggy::StopGroggyDrain(UAbilitySystemComponent& ASC)
{
	if (DrainGPEffectHandle.IsValid())
	{
		ASC.RemoveActiveGameplayEffect(DrainGPEffectHandle);
		DrainGPEffectHandle.Invalidate();
	}

	if (GPDelegateHandle.IsValid())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetGPAttribute()).Remove(GPDelegateHandle);
		GPDelegateHandle.Reset();
	}
}

void UWxAbility_Groggy::SetAILogicPaused(const FGameplayAbilityActorInfo* ActorInfo, bool bPaused) const
{
	APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	AAIController* AIController = AvatarPawn ? Cast<AAIController>(AvatarPawn->GetController()) : nullptr;
	UBrainComponent* Brain = AIController ? AIController->GetBrainComponent() : nullptr;
	if (!Brain)
	{
		return;
	}

	if (bPaused)
	{
		Brain->PauseLogic(TEXT("Groggy"));
	}
	else
	{
		Brain->ResumeLogic(TEXT("Groggy"));
	}
}
