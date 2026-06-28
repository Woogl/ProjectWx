// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "WxCombatLibrary.h"
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

	// 트리거로 변형을 분기한다. 공격 몽타주·짝 피격 태그·대미지 타이밍 태그만 변형별로 다르고 나머지 흐름은 공유한다.
	const bool bBackstab = (TriggerTag == WxGameplayTags::Event_Backstab);
	UAnimMontage* AttackerMontage = bBackstab ? BackstabMontage : FinisherMontage;
	const FGameplayTag VictimHitReactTag = bBackstab ? WxGameplayTags::Event_HitReact_Backstab : WxGameplayTags::Event_HitReact_Finisher;
	const FGameplayTag DamageEventTag = bBackstab ? WxGameplayTags::Event_Backstab_Damage : WxGameplayTags::Event_Finisher_Damage;

	if (!AttackerMontage || !AvatarActor || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = Target;

	// 1. 대상 적 쪽으로 모션워핑 정렬(몽타주의 MotionWarping 노티파이가 이 워프 타겟을 소비한다).
	RegisterWarpTarget(AvatarActor, Target);

	// 2. 적에게 짝 피격 이벤트 송출 → HitReact 가 짝 피격 몽타주를 공격 몽타주와 동시 재생.
	FGameplayEventData VictimEvent;
	VictimEvent.Instigator = AvatarActor;
	VictimEvent.Target = Target;
	VictimEvent.EventTag = VictimHitReactTag;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, VictimHitReactTag, VictimEvent);

	// 3. 대미지 타이밍 이벤트 대기(몽타주 후반 노티파이가 1회 송출).
	DamageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, DamageEventTag, nullptr, true, true);
	DamageEventTask->EventReceived.AddDynamic(this, &UWxAbility_Finisher::HandleDamageEvent);
	DamageEventTask->ReadyForActivation();

	// 4. 공격자 몽타주 재생.
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
	if (DamageEventTask)
	{
		DamageEventTask->EndTask();
		DamageEventTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Finisher::HandleDamageEvent(FGameplayEventData Payload)
{
	// 어떤 변형의 대미지 노티파이가 도착했는지는 Payload 의 EventTag 로 판별한다(별도 상태 보관 없음).
	if (Payload.EventTag == WxGameplayTags::Event_Backstab_Damage)
	{
		ApplyBackstabDamage();
	}
	else
	{
		ApplyFinisherDamage();
	}
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

	// SnapToTarget 과 동일한 수평(yaw) 접근/정렬 규칙. 대상 WarpDistance 지점에서 멈추고 대상을 바라본다.
	// 공격자가 앞(앞잡)이든 뒤(뒤잡)든 "공격자→대상 방향"을 그대로 따르므로 변형 공통이다(뒤잡은 어포던스가 후방을 보장).
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
	const FRotator WarpRotation = Direction.Rotation();

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);
}

void UWxAbility_Finisher::ApplyFinisherDamage() const
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

	UWxCombatLibrary::ApplyDamage(SourceASC, TargetASC, DamageInfo, HitResult, 0.f);
}

void UWxAbility_Finisher::ApplyBackstabDamage() const
{
	AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}

	// 고정 치명 대미지로 HP 0 → State.Dead → HandleDeath(보상·AI 정지). 짝 피격 몽타주는 발동 즉시 시작돼 이미 재생 중이다.
	UWxCombatLibrary::ApplyRawDamage(TargetASC, BackstabDamage, FGameplayTag());
}
