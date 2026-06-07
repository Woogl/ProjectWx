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

void UWxAbility_LockOn::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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

	// 락온 중에는 이동 방향 회전을 끈다. 캐릭터를 타겟으로 향하게 하는 회전은 태스크가 부드럽게 보간한다. EndAbility에서 복구.
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	// 락온 태스크 생성
	LockOnTask = UWxAbilityTask_LockOnTarget::CreateTask(this, FoundTarget, CameraInterpSpeed, CameraPitchOffset, MaxDistance, CharacterInterpSpeed, ReticleWidgetClass);
	LockOnTask->OnTargetLost.AddDynamic(this, &UWxAbility_LockOn::HandleTargetLost);
	LockOnTask->ReadyForActivation();
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

		// 락온 해제 시 OrientToMovement 복구. 이동 시 RotationRate로 부드럽게 이동 방향으로 돌아온다.
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}

	LockOnTask = nullptr;
}

void UWxAbility_LockOn::HandleTargetLost()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
