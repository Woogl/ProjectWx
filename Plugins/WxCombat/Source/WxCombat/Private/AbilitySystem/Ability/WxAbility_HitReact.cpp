// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// 항상 서버 ExecCalc의 GameplayEvent로 트리거된다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_HitReact);
	SetAssetTags(AssetTags);

	// 피격은 반응이 끝날 때까지 새 액션을 막는다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	// 진행 중인 것은 공격·스킬만 끊는다 — 마커로 끊으면 마커를 가진 적 패턴까지 평타 피격에 중단된다.
	// 차단만으로는 부족하다: 공격·스킬의 콤보 재발동 분기는 활성 Spec만 보고 자체 판정하므로 ASC의 차단 태그 검사를 건너뛴다.
	// Ability.Skill은 부모 태그라 슬롯별 Ability.Skill.1~4까지 함께 잡는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);

	ActivationOwnedTags.AddTag(WxGameplayTags::State_HitReact);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Guard);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_SuperArmor);

	bRetriggerInstancedAbility = true;

	// AbilityTriggers는 정확한 태그 매칭이라 종류마다 개별 등록해야 한다.
	FAbilityTriggerData NormalTrigger;
	NormalTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Normal;
	NormalTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(NormalTrigger);

	FAbilityTriggerData KnockbackTrigger;
	KnockbackTrigger.TriggerTag = WxGameplayTags::Event_HitReact_KnockBack;
	KnockbackTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(KnockbackTrigger);

	FAbilityTriggerData KnockdownTrigger;
	KnockdownTrigger.TriggerTag = WxGameplayTags::Event_HitReact_KnockDown;
	KnockdownTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(KnockdownTrigger);

	FAbilityTriggerData KnockupTrigger;
	KnockupTrigger.TriggerTag = WxGameplayTags::Event_HitReact_KnockUp;
	KnockupTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(KnockupTrigger);

	FAbilityTriggerData ParryTrigger;
	ParryTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Parry;
	ParryTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(ParryTrigger);

	FAbilityTriggerData FinisherTrigger;
	FinisherTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Finisher;
	FinisherTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(FinisherTrigger);

	FAbilityTriggerData BackstabTrigger;
	BackstabTrigger.TriggerTag = WxGameplayTags::Event_HitReact_Backstab;
	BackstabTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(BackstabTrigger);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const FGameplayTag EventTag = TriggerEventData ? TriggerEventData->EventTag : WxGameplayTags::Event_HitReact_Normal;

	// 매칭 몽타주가 없으면 NormalHitReactMontage로 폴백한다.
	UAnimMontage* SelectedMontage = NormalHitReactMontage;
	if (TriggerEventData)
	{
		if (EventTag == WxGameplayTags::Event_HitReact_KnockBack && KnockbackMontage)
		{
			SelectedMontage = KnockbackMontage;
			FaceInstigator(ActorInfo->AvatarActor.Get(), TriggerEventData->Instigator.Get());
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_KnockDown && KnockdownMontage)
		{
			SelectedMontage = KnockdownMontage;
			FaceInstigator(ActorInfo->AvatarActor.Get(), TriggerEventData->Instigator.Get());
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_KnockUp && KnockupMontage)
		{
			SelectedMontage = KnockupMontage;

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
		else if (EventTag == WxGameplayTags::Event_HitReact_Finisher && FinisherHitReactMontage)
		{
			SelectedMontage = FinisherHitReactMontage;
			FaceInstigator(ActorInfo->AvatarActor.Get(), TriggerEventData->Instigator.Get());
		}
		else if (EventTag == WxGameplayTags::Event_HitReact_Backstab && BackstabHitReactMontage)
		{
			// 뒤잡은 몬스터를 공격자 쪽으로 돌려세운 뒤 처형한다.
			SelectedMontage = BackstabHitReactMontage;
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
	// 재진입(Normal → Knockback 등)에서 구 태스크를 남겨 두면, 잔여 콜백이 새로 시작한 재생을 즉시 끝내 버린다.
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

void UWxAbility_HitReact::FaceInstigator(AActor* AvatarActor, const AActor* Instigator)
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
