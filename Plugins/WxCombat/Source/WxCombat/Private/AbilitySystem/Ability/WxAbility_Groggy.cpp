// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_DrainDP.h"
#include "AbilitySystem/Effect/WxEffect_ResetDP.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

UWxAbility_Groggy::UWxAbility_Groggy()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Groggy);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Groggy);
	
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 그로기에 빠지면 진행 중이던 액션(적 패턴 포함)을 끊고 그로기 동안 새 액션도 막는다.
	ActivationGroup = EWxAbilityActivationGroup::Reaction;
	bCancelsRunningActions = true;

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

	// 그대로 두면 폴러가 사망 몽타주 종료 후 시체 위에 그로기 몽타주를 덮어씌운다.
	DeadTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxAbility_Groggy::HandleDeadTagChanged);

	DPDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetDPAttribute())
		.AddUObject(this, &UWxAbility_Groggy::HandleDPChanged);

	// 재생 rate를 1.0으로 고정하므로 몽타주 길이가 곧 그로기 길이다.
	const float GroggyDuration = GroggyMontage->GetPlayLength();

	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainDP::StaticClass(), GetAbilityLevel());
	if (DrainSpecHandle.IsValid())
	{
		DrainSpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, GroggyDuration);
		DrainDPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
	}

	if (UWorld* World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(MontagePollingTimerHandle, this, &UWxAbility_Groggy::HandleMontagePollTick, 0.1f, true);

		World->GetTimerManager().SetTimer(GroggySafetyTimerHandle, this, &UWxAbility_Groggy::HandleGroggySafetyTimeout, GroggyDuration + 1.f, false);
	}

	if (APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
	{
		if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
		{
			if (UBrainComponent* Brain = AIController->GetBrainComponent())
			{
				Brain->PauseLogic(TEXT("Groggy"));
			}
		}
	}

	HandleMontagePollTick();
}

void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo)
	{
		if (UWorld* World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(MontagePollingTimerHandle);
			World->GetTimerManager().ClearTimer(GroggySafetyTimerHandle);
		}

		if (APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
		{
			if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
			{
				if (UBrainComponent* Brain = AIController->GetBrainComponent())
				{
					Brain->ResumeLogic(TEXT("Groggy"));
				}
			}
		}

		if (ActorInfo->AbilitySystemComponent.IsValid())
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

			// ActivateAbility가 GroggyMontage 미설정으로 곧장 EndAbility를 부르는 경로가 있어 널일 수 있다.
			if (GroggyMontage)
			{
				ASC->StopMontageIfCurrent(*GroggyMontage);
			}

			if (DrainDPEffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(DrainDPEffectHandle);
				DrainDPEffectHandle.Invalidate();
			}

			if (DeadTagDelegateHandle.IsValid())
			{
				ASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
					.Remove(DeadTagDelegateHandle);
				DeadTagDelegateHandle.Reset();
			}

			if (DPDelegateHandle.IsValid())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetDPAttribute())
					.Remove(DPDelegateHandle);
				DPDelegateHandle.Reset();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Groggy::HandleDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Groggy::HandleDPChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::HandleGroggySafetyTimeout()
{
	// 여기서 곧장 EndAbility만 하면 DP가 MaxDP로 남아, 다음 DP 변동에서 AttributeSet이 Event.Groggy를 다시 송출한다.
	const FGameplayEffectSpecHandle ResetSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_ResetDP::StaticClass(), GetAbilityLevel());
	if (ResetSpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, ResetSpecHandle);
	}
}

void UWxAbility_Groggy::HandleMontagePollTick()
{
	if (!CurrentActorInfo)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	if (ASC->GetCurrentMontage() != nullptr)
	{
		return;
	}

	ASC->PlayMontage(this, CurrentActivationInfo, GroggyMontage, 1.f);
}
