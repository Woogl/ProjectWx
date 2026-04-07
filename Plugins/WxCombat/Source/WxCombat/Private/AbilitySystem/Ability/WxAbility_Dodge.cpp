// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystem/Effect/WxEffect_RecoveryMP.h"
#include "AbilitySystem/TargetData/WxAbilityTargetData_Direction.h"
#include "AbilitySystem/Task/WxAbilityTask_TurnAround.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

UWxAbility_Dodge::UWxAbility_Dodge()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Dodge);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	ActivationInputTag = WxGameplayTags::Input_Dodge;
	CooldownTag = WxGameplayTags::Cooldown_Dodge;

	CooldownDuration = 2.f;
	MaxCharges = 2;
}

void UWxAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (IsLocallyControlled())
	{
		// 로컬 클라이언트(또는 리슨 서버 호스트): 입력 방향을 직접 읽어 회전 적용
		FVector DodgeDirection = FVector::ZeroVector;
		if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			DodgeDirection = Character->GetLastMovementInputVector();
		}

		ApplyDodgeDirection(DodgeDirection);

		// 리모트 클라이언트인 경우 서버에 방향 전송
		if (ASC && !HasAuthority(&ActivationInfo))
		{
			FGameplayAbilityTargetDataHandle DataHandle;
			FWxAbilityTargetData_Direction* DirectionData = new FWxAbilityTargetData_Direction();
			DirectionData->Direction = DodgeDirection;
			DataHandle.Add(DirectionData);

			ASC->CallServerSetReplicatedTargetData(
				Handle,
				ActivationInfo.GetActivationPredictionKey(),
				DataHandle,
				FGameplayTag(),
				ASC->ScopedPredictionKey);
		}
	}
	else if (HasAuthority(&ActivationInfo))
	{
		// 서버(리모트 플레이어 처리): 클라이언트로부터 방향 데이터 수신 대기
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(
				Handle,
				ActivationInfo.GetActivationPredictionKey());
			Delegate.AddUObject(this, &UWxAbility_Dodge::HandleTargetDataReceived);

			ASC->CallReplicatedTargetDataDelegatesIfSet(
				Handle,
				ActivationInfo.GetActivationPredictionKey());
		}
	}

	if (!DodgeMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DodgeMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	ListenForDodgeSuccess();
}

void UWxAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	// 극한 회피 중 추가한 공격 입력 태그 제거
	if (bPlayingPerfectDodge && DodgeCounterMontage && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec)
		{
			Spec->GetDynamicSpecSourceTags().RemoveTag(WxGameplayTags::Input_Attack);
			ActorInfo->AbilitySystemComponent->MarkAbilitySpecDirty(*Spec);
		}
	}

	bPlayingPerfectDodge = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ────────────────────────────────────────────────────────────────────────────
//  방향 처리
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::ApplyDodgeDirection(const FVector& Direction)
{
	if (!Direction.IsNearlyZero())
	{
		UWxAbilityTask_TurnAround* TurnAroundTask = UWxAbilityTask_TurnAround::CreateTask(this, Direction);
		TurnAroundTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	const FWxAbilityTargetData_Direction* DirectionData =
		static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0));
	if (DirectionData)
	{
		ApplyDodgeDirection(DirectionData->Direction);
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}
}

// ────────────────────────────────────────────────────────────────────────────
//  극한 회피 처리
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::ListenForDodgeSuccess()
{
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_DodgeSuccess);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Dodge::HandleDodgeSuccess);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::HandleDodgeSuccess(FGameplayEventData Payload)
{
	if (!PerfectDodgeMontage)
	{
		return;
	}

	// 극한 회피 성공 보상: MP 회복
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		const UGameplayEffect* Effect = UWxEffect_RecoveryMP::StaticClass()->GetDefaultObject<UGameplayEffect>();
		FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.f);
		Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery, 5.f);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
	}

	bPlayingPerfectDodge = true;
	PlayPerfectDodgeMontage();

	if (DodgeCounterMontage)
	{
		// 공격 입력 태그를 스펙에 동적 추가하여, ASC의 입력 라우팅이 이 어빌리티에 도달하도록 함
		if (ASC)
		{
			FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
			if (Spec)
			{
				Spec->GetDynamicSpecSourceTags().AddTag(WxGameplayTags::Input_Attack);
				ASC->MarkAbilitySpecDirty(*Spec);
			}
		}

		ListenForCounterInput();
	}
}

void UWxAbility_Dodge::PlayPerfectDodgeMontage()
{
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
		MontageTask->OnBlendOut.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
		MontageTask->EndTask();
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, PerfectDodgeMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

// ────────────────────────────────────────────────────────────────────────────
//  반격 처리
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::ListenForCounterInput()
{
	WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	if (WaitInputTask)
	{
		WaitInputTask->OnPress.AddDynamic(this, &UWxAbility_Dodge::HandleCounterInputPressed);
		WaitInputTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::HandleCounterInputPressed(float TimeWaited)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// ANS_ComboWindow 구간 내에서만 반격 허용
	if (!ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		ListenForCounterInput();
		return;
	}

	PlayDodgeCounterMontage();
}

void UWxAbility_Dodge::PlayDodgeCounterMontage()
{
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
		MontageTask->OnBlendOut.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
		MontageTask->EndTask();
	}

	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DodgeCounterMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

// ────────────────────────────────────────────────────────────────────────────
//  몽타주 콜백
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Dodge::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Dodge::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Dodge::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
