// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_LockOn.h"
#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"
#include "WxGameplayTags.h"

UWxAbility_LockOn::UWxAbility_LockOn()
{
	// AssetTag 의도적 미설정: Ability 태그가 없어야 HitReact/Guard 등의
	// CancelAbilitiesWithTag(Ability)에 의해 락온이 해제되지 않는다.
	ActivationInputTag = WxGameplayTags::Input_LockOn;
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationOwnedTags.AddTag(WxGameplayTags::State_LockOn);
}

void UWxAbility_LockOn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TargetingPreset || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// TargetingSubsystem으로 동기 타겟 탐색
	UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetWorld());
	if (!TargetingSubsystem)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = GetOwningActorFromActorInfo();
	FTargetingRequestHandle RequestHandle = UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(RequestHandle, FTargetingRequestDelegate());

	// 결과에서 가장 가까운 타겟 추출
	FTargetingDefaultResultsSet& ResultsSet = FTargetingDefaultResultsSet::FindOrAdd(RequestHandle);
	TArray<FTargetingDefaultResultData>& Results = ResultsSet.TargetResults;
	if (Results.IsEmpty())
	{
		UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* FoundTarget = Results[0].HitResult.GetActor();
	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);

	if (!FoundTarget)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 락온 대상 등록
	if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(GetOwningActorFromActorInfo()))
	{
		LockOnComp->SetLockOnTarget(FoundTarget);
	}

	// 락온 중에는 이동 방향이 아닌 타겟 기준으로 회전한다.
	// OrientToMovement를 끄고, 태스크가 타겟을 향해 보간하는 컨트롤러 yaw를 따르게 한다. EndAbility에서 복구.
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}

	// 락온 태스크 생성
	LockOnTask = UWxAbilityTask_LockOnTarget::CreateTask(this, FoundTarget, CameraInterpSpeed, CameraPitchOffset, MaxDistance, ReticleWidgetClass);
	LockOnTask->OnTargetLost.AddDynamic(this, &UWxAbility_LockOn::HandleTargetLost);
	LockOnTask->ReadyForActivation();
}

void UWxAbility_LockOn::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_LockOn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(GetOwningActorFromActorInfo()))
		{
			LockOnComp->SetLockOnTarget(nullptr);
		}

		// 락온 해제 시 회전 설정을 기본값으로 복구.
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = true;
			Character->bUseControllerRotationYaw = false;
		}
	}

	LockOnTask = nullptr;
}

void UWxAbility_LockOn::HandleTargetLost()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
