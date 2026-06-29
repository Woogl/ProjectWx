// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "WxCombatLibrary.h"
#include "WxDamageInfo.h"
#include "WxGameplayTags.h"

const FName UWxAbility_Finisher::WarpTargetName = TEXT("Finisher");

UWxAbility_Finisher::UWxAbility_Finisher()
{
	// 발동은 상호작용(서버 권위)이 보내는 GameplayEvent 다. 대미지도 서버에서 적용하므로 ServerInitiated.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Finisher);
	SetAssetTags(AssetTags);

	// 피니시 연출 중에는 다른 어빌리티로 캔슬되지 않게 자기 차단한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 변형별 트리거. 앞잡(Event.Finisher)과 뒤잡(Event.Backstab)을 한 어빌리티가 받는다.
	FAbilityTriggerData FinisherTrigger;
	FinisherTrigger.TriggerTag = WxGameplayTags::Event_Finisher;
	FinisherTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(FinisherTrigger);

	FAbilityTriggerData BackstabTrigger;
	BackstabTrigger.TriggerTag = WxGameplayTags::Event_Backstab;
	BackstabTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(BackstabTrigger);
}

void UWxAbility_Finisher::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	AActor* Target = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	const FGameplayTag TriggerTag = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();

	// 트리거로 변형을 분기한다. 공격 몽타주·짝 피격 태그만 변형별로 다르고 나머지 흐름은 공유한다.
	// 대미지 수치·타이밍은 어빌리티가 아니라 공격 몽타주의 WxAnimNotify_FinisherDamage 가 결정한다.
	const bool bBackstab = (TriggerTag == WxGameplayTags::Event_Backstab);
	UAnimMontage* AttackerMontage = bBackstab ? BackstabMontage : FinisherMontage;
	const FGameplayTag VictimHitReactTag = bBackstab ? WxGameplayTags::Event_HitReact_Backstab : WxGameplayTags::Event_HitReact_Finisher;

	if (!AttackerMontage || !AvatarActor || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = Target;

	// 1. 대상 적 현재 위치 앞으로 플레이어 위치를 모션워핑 정렬(회전은 플레이어 방향 유지). 몽타주의 MotionWarping 노티파이가 소비한다.
	RegisterWarpTarget(AvatarActor, Target);

	// 2. 적에게 짝 피격 이벤트 송출 → HitReact 가 짝 피격 몽타주를 공격 몽타주와 동시 재생.
	FGameplayEventData VictimEvent;
	VictimEvent.Instigator = AvatarActor;
	VictimEvent.Target = Target;
	VictimEvent.EventTag = VictimHitReactTag;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, VictimHitReactTag, VictimEvent);

	// 3. 공격자 몽타주 재생(몽타주의 WxAnimNotify_FinisherDamage 가 ApplyFinisherDamage 를 호출해 대미지 적용).
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackerMontage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Finisher::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Finisher::HandleMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Finisher::HandleMontageFinished);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Finisher::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Finisher::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Finisher::RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const
{
	UMotionWarpingComponent* MotionWarping = AvatarActor ? AvatarActor->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!MotionWarping || !Target)
	{
		return;
	}

	// 위치만 정렬한다: 대상(몬스터) 현재 위치 기준으로 WarpDistance 지점까지 접근하되, 회전은 플레이어
	// 현재 방향을 유지한다(각도 기준=플레이어). 몬스터가 플레이어를 향해 회전하는 것은 HitReact 짝 피격이 담당한다.
	const FVector OwnerLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();
	FVector Direction = TargetLocation - OwnerLocation;
	Direction.Z = 0.0;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const float Distance = Direction.Size();
	const FVector DirectionNorm = Direction / Distance;
	const float StopDistance = FMath::Max(0.f, Distance - WarpDistance);
	const FVector WarpLocation = OwnerLocation + DirectionNorm * StopDistance;
	const FRotator WarpRotation = AvatarActor->GetActorRotation();

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);
}

void UWxAbility_Finisher::ApplyFinisherDamage(const FWxDamageInfo& DamageInfo) const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.Location = Target->GetActorLocation();

	// 상호작용으로 확정한 대상에 노티파이가 넘긴 피해를 적용한다. 앞잡·뒤잡 공통 경로.
	// 앞잡 그로기 해제(DP 0)·뒤잡 즉사(CoeffATK)는 노티파이의 대미지 행 데이터로 결정된다.
	UWxCombatLibrary::ApplyDamage(SourceASC, TargetASC, DamageInfo, HitResult, 0.f);
}
