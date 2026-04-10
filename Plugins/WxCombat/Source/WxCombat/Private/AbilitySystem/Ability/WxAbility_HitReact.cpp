// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// HitReact는 항상 서버의 ExecCalc에서 GameplayEvent로 트리거되므로 ServerInitiated를 사용한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Guard);

	bRetriggerInstancedAbility = true;

	// AbilityTriggers는 정확한 태그 매칭을 사용하므로 각 HitReact 종류를 개별 등록한다.
	FAbilityTriggerData NormalTrigger;
	NormalTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Normal;
	NormalTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(NormalTrigger);

	FAbilityTriggerData KnockbackTrigger;
	KnockbackTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Knockback;
	KnockbackTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(KnockbackTrigger);

	FAbilityTriggerData KnockdownTrigger;
	KnockdownTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Knockdown;
	KnockdownTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(KnockdownTrigger);

	FAbilityTriggerData KnockupTrigger;
	KnockupTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Knockup;
	KnockupTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(KnockupTrigger);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 그로기 상태가 아니면 PP를 MaxPP만큼 회복
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		if (const UWxCombatAttributeSet* AttributeSet = ASC->GetSet<UWxCombatAttributeSet>())
		{
			ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetPPAttribute(), AttributeSet->GetMaxPP());
		}
	}

	// 트리거 태그에 따라 재생할 몽타주 선택. 매칭 몽타주가 없으면 기본 HitReactMontage로 폴백.
	UAnimMontage* SelectedMontage = HitReactMontage;
	if (TriggerEventData)
	{
		const FGameplayTag& EventTag = TriggerEventData->EventTag;
		if (EventTag == WxGameplayTags::Event_HitReact_Knockback && KnockbackMontage)
		{
			SelectedMontage = KnockbackMontage;

			// 넉백 시 공격자를 바라보도록 회전
			if (AActor* AvatarActor = ActorInfo->AvatarActor.Get())
			{
				if (const AActor* Instigator = TriggerEventData->Instigator.Get())
				{
					FVector Direction = Instigator->GetActorLocation() - AvatarActor->GetActorLocation();
					Direction.Z = 0.0;
					if (!Direction.IsNearlyZero())
					{
						AvatarActor->SetActorRotation(Direction.ToOrientationRotator());
					}
				}
			}
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_Knockdown && KnockdownMontage)
		{
			SelectedMontage = KnockdownMontage;
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_Knockup && KnockupMontage)
		{
			SelectedMontage = KnockupMontage;
		}
	}

	if (!SelectedMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!PlayHitReactMontage(SelectedMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

void UWxAbility_HitReact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CurrentMontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWxAbility_HitReact::PlayHitReactMontage(UAnimMontage* Montage)
{
	// 재진입(Normal → Knockback 등) 시 이전 몽타주 태스크를 명시적으로 정리해
	// 잔여 OnInterrupted/OnCancelled 콜백이 새로 시작된 재생을 즉시 종료시키는 레이스를 차단한다.
	if (CurrentMontageTask)
	{
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	CurrentMontageTask = MontageTask;

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
	return true;
}

void UWxAbility_HitReact::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_HitReact::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_HitReact::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
