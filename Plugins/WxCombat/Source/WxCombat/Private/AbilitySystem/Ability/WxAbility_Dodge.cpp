// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
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

	ActivationInputTag = WxGameplayTags::Input_Dodge;
	CooldownTag = WxGameplayTags::Cooldown_Dodge;
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

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
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
}

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
