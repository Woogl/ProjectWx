// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxGameplayTags.h"

namespace
{
	void FaceInstigator(AActor* AvatarActor, const AActor* Instigator)
	{
		if (!AvatarActor || !Instigator)
		{
			return;
		}

		FVector Direction = Instigator->GetActorLocation() - AvatarActor->GetActorLocation();
		Direction.Z = 0.0;
		if (!Direction.IsNearlyZero())
		{
			AvatarActor->SetActorRotation(Direction.ToOrientationRotator());
		}
	}
}

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// HitReact는 항상 서버의 ExecCalc에서 GameplayEvent로 트리거되므로 ServerInitiated를 사용한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 피격 반응은 Attack/Skill이 건 BlockAbilitiesWithTag(Ability)에 절대 막히면 안 되는 반응 어빌리티다.
	// 차단은 상대 AssetTag가 Ability.*에 매칭될 때만 걸리므로, 매칭될 태그가 없도록 AssetTag를 비워 둔다.
	// 피격 시 플레이어가 진행 중인 공격·스킬만 캔슬한다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);
	// 피격 시 적의 패턴은 캔슬하지 않으므로 주석 처리
	// CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Pattern);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationOwnedTags.AddTag(WxGameplayTags::State_HitReact);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Guard);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_SuperArmor);

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

	FAbilityTriggerData ParryTrigger;
	ParryTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Parry;
	ParryTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(ParryTrigger);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const FGameplayTag EventTag = TriggerEventData ? TriggerEventData->EventTag : WxGameplayTags::Event_HitReact_Normal;

	// // 적 패턴(Ability.Pattern) 발동 중에는 일반(Normal) 피격 반응을 발생시키지 않는다.
	// // 강한 피격(넉백·넉다운·넉업·패리)은 패턴 중에도 그대로 반응한다.
	// if (EventTag == WxGameplayTags::Event_HitReact_Normal)
	// {
	// 	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	// 	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Pattern))
	// 	{
	// 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	// 		return;
	// 	}
	// }

	// 트리거 태그에 따라 재생할 몽타주 선택. 매칭 몽타주가 없으면 기본 HitReactMontage로 폴백.
	UAnimMontage* SelectedMontage = HitReactMontage;
	if (TriggerEventData)
	{
		if (EventTag == WxGameplayTags::Event_HitReact_Knockback && KnockbackMontage)
		{
			SelectedMontage = KnockbackMontage;
			FaceInstigator(ActorInfo->AvatarActor.Get(), TriggerEventData->Instigator.Get());
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_Knockdown && KnockdownMontage)
		{
			SelectedMontage = KnockdownMontage;
			FaceInstigator(ActorInfo->AvatarActor.Get(), TriggerEventData->Instigator.Get());
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_Knockup && KnockupMontage)
		{
			SelectedMontage = KnockupMontage;

			// 넉업 시 공중에 띄움
			if (AActor* AvatarActor = ActorInfo->AvatarActor.Get())
			{
				if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
				{
					FVector LaunchVelocity = FVector(0.f, 0.f, Character->GetCharacterMovement()->JumpZVelocity);
					Character->LaunchCharacter(LaunchVelocity, false, true);
				}
			}
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_Parry && ParryReactMontage)
		{
			SelectedMontage = ParryReactMontage;
			FaceInstigator(ActorInfo->AvatarActor.Get(), TriggerEventData->Instigator.Get());
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
	if (ActorInfo)
	{
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->MovementModeChangedDelegate.RemoveAll(this);
		}
	}

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

	if (Montage == KnockupMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()))
		{
			Character->MovementModeChangedDelegate.AddDynamic(this, &UWxAbility_HitReact::HandleMovementModeChanged);
		}
	}

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

void UWxAbility_HitReact::HandleMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (PrevMovementMode == MOVE_Falling)
	{
		Character->MovementModeChangedDelegate.RemoveAll(this);

		if (UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(KnockupMontage))
			{
				AnimInstance->Montage_JumpToSection(FName(TEXT("Grounded")), KnockupMontage);
			}
		}
	}
}
