// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
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

	// 그로기에 빠지면 진행 중이던 액션(적 패턴 포함)을 끊고 그로기 동안 새 액션도 막는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::State_Groggy;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
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

	GroggyTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxAbility_Groggy::HandleGroggyTagChanged);

	// 그대로 두면 폴러가 사망 몽타주 종료 후 시체 위에 그로기 몽타주를 덮어씌운다.
	DeadTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxAbility_Groggy::HandleDeadTagChanged);

	// 그로기 길이는 몽타주 재생 길이를 따른다(재생 rate는 1.0 고정).
	const float GroggyDuration = GroggyMontage->GetPlayLength();

	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainDP::StaticClass(), GetAbilityLevel());
	if (DrainSpecHandle.IsValid())
	{
		// DP는 이 시간 동안 0까지 드레인되며, 0에 도달해 State.Groggy 태그가 제거되면 그로기가 끝난다.
		DrainSpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, GroggyDuration);
		DrainDPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
	}

	if (UWorld* World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(MontagePollingTimerHandle, this, &UWxAbility_Groggy::HandleMontagePollTick, 0.1f, true);

		// 실패복구: DrainDP가 무효·외부 제거로 DP를 0까지 못 내리면 State.Groggy가 잔존해 무한 그로기가 된다.
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

			if (GroggyTagDelegateHandle.IsValid())
			{
				ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
					.Remove(GroggyTagDelegateHandle);
				GroggyTagDelegateHandle.Reset();
			}

			if (DeadTagDelegateHandle.IsValid())
			{
				ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
					.Remove(DeadTagDelegateHandle);
				DeadTagDelegateHandle.Reset();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Groggy::HandleGroggyTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount == 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::HandleDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Groggy::HandleGroggySafetyTimeout()
{
	// DP를 리셋하면 AttributeSet가 State.Groggy를 떼고, 그 변화가 HandleGroggyTagChanged로 정상 종료 경로를 태운다.
	// 여기서 곧장 EndAbility만 하면 DP가 MaxDP로 남아 OwnedTagPresent 트리거가 즉시 재발동한다.
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
