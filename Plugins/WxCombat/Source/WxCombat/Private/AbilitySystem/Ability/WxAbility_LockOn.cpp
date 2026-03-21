// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_LockOn.h"
#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"
#include "WxGameplayTags.h"

UWxAbility_LockOn::UWxAbility_LockOn()
{
	// AbilityTags 의도적 미설정: Ability 태그가 없어야 HitReact/Guard 등의
	// CancelAbilitiesWithTag(Ability)에 의해 락온이 해제되지 않는다.
	ActivationInputTag = WxGameplayTags::Input_LockOn;
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
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

	// State_LockOn 태그 추가 및 락온 대상 등록
	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(WxGameplayTags::State_LockOn);
	if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
	{
		WxASC->SetLockOnTarget(FoundTarget);
	}

	// 락온 태스크 생성
	LockOnTask = UWxAbilityTask_LockOnTarget::CreateTask(this, FoundTarget, CameraInterpSpeed, MaxPitchOffset, MaxDistance, ReticleWidgetClass);
	LockOnTask->OnTargetLost.AddDynamic(this, &UWxAbility_LockOn::HandleTargetLost);
	LockOnTask->ReadyForActivation();
}

void UWxAbility_LockOn::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_LockOn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(WxGameplayTags::State_LockOn);
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
		{
			WxASC->SetLockOnTarget(nullptr);
		}
	}

	LockOnTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_LockOn::HandleTargetLost()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
