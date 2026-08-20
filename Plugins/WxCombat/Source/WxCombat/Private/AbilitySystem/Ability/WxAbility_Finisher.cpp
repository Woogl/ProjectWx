// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "AbilitySystem/Effect/WxEffect_ResetDP.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Damage/WxDamageInfo.h"
#include "MotionWarpingComponent.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"

namespace
{
	// 두 변형의 공격 몽타주 MotionWarping 노티파이 Warp Target Name을 이 값으로 맞춘다.
	const FName WarpTargetName = TEXT("Finisher");
}

UWxAbility_Finisher::UWxAbility_Finisher()
{
	// 발동도 대미지 적용도 서버 권위이므로 ServerInitiated.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Finisher);
	SetAssetTags(AssetTags);

	// 앞잡은 플레이어의 공격으로 만들어지므로, 그로기 직후 F를 누르면 그 공격이 아직 본동작일 수 있다(막힌 발동은 재시도 없이 소모).
	// Reaction은 그 점유에 막히지 않고, 발동하며 진행 중이던 액션을 끊는다.
	ActivationGroup = EWxAbilityActivationGroup::Reaction;
	bCancelsRunningActions = true;

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);
	ActivationOwnedEffects.Add(UWxEffect_Invincible::StaticClass());

	// 상호작용이 이 태그에 막혀, 연출 도중 재입력으로 다른 대상과 몽타주가 겹치는 것을 차단한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Finisher);

	FAbilityTriggerData FinisherTrigger;
	FinisherTrigger.TriggerTag = WxGameplayTags::Event_Finisher;
	FinisherTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(FinisherTrigger);

	FAbilityTriggerData BackstabTrigger;
	BackstabTrigger.TriggerTag = WxGameplayTags::Event_Backstab;
	BackstabTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(BackstabTrigger);
}

float UWxAbility_Finisher::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_Finisher::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	// 대상에 가하는 변경은 전부 대상 ASC 를 거치고 액터 자체는 위치만 읽으므로 const 로 다룬다.
	const AActor* Target = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
	const FGameplayTag TriggerTag = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();

	const bool bBackstab = (TriggerTag == WxGameplayTags::Event_Backstab);
	UAnimMontage* AttackerMontage = bBackstab ? BackstabMontage.Get() : FinisherMontage.Get();
	const FGameplayTag VictimHitReactTag = bBackstab ? WxGameplayTags::Event_HitReact_Backstab : WxGameplayTags::Event_HitReact_Finisher;

	if (!AttackerMontage || !AvatarActor || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = Target;

	// 연출 진행 상태를 대상에게도 발행한다(권위 발행 + 복제).
	// 대상이 이 태그로 자기 처형 어포던스를 닫아 프롬프트 재노출·중복 발동을 막는다.
	if (ActorInfo->IsNetAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			TargetASC->AddLooseGameplayTag(WxGameplayTags::State_BeingFinished, 1, EGameplayTagReplicationState::TagOnly);
		}
	}

	RegisterWarpTarget(AvatarActor, Target);

	// 짝 피격 이벤트를 받은 HitReact가 피격 몽타주를 공격 몽타주와 동시에 재생한다.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		FGameplayEventData VictimEvent;
		VictimEvent.Instigator = AvatarActor;
		VictimEvent.Target = Target;
		VictimEvent.EventTag = VictimHitReactTag;
		TargetASC->HandleGameplayEvent(VictimHitReactTag, &VictimEvent);
	}

	if (!PlayMontage(AttackerMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Finisher::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 중단·캔슬도 이 경로를 지나므로 대상에 걸어둔 연출 태그가 새지 않고, 그로기 해제도 몽타주 종료 방식과 무관하게 한 번 일어난다.
	// 대상이 대미지를 견뎌 살아남으면 여기서 어포던스가 복구된다.
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor.Get()))
		{
			if (UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get())
			{
				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				const FGameplayEffectSpecHandle ResetSpec = SourceASC->MakeOutgoingSpec(UWxEffect_ResetDP::StaticClass(), GetAbilityLevel(), Context);
				if (ResetSpec.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*ResetSpec.Data.Get(), TargetASC);
				}
			}

			TargetASC->RemoveLooseGameplayTag(WxGameplayTags::State_BeingFinished, 1, EGameplayTagReplicationState::TagOnly);
		}
	}
	TargetActor = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Finisher::RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const
{
	UMotionWarpingComponent* MotionWarping = AvatarActor ? AvatarActor->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!MotionWarping || !Target)
	{
		return;
	}

	// 멈출 간격·상대 포즈는 공격 몽타주의 Motion Warping Warp Point(애니)가 소유한다.
	// 몬스터가 플레이어를 향해 회전하는 것은 HitReact 짝 피격이 담당한다.
	const FVector TargetLocation = Target->GetActorLocation();
	FVector Direction = TargetLocation - AvatarActor->GetActorLocation();
	Direction.Z = 0.0;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator WarpRotation = Direction.Rotation();
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, TargetLocation, WarpRotation);
}

void UWxAbility_Finisher::ApplyFinisherDamage(const FWxDamageInfo& DamageInfo) const
{
	const AActor* Target = TargetActor.Get();
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.Location = Target->GetActorLocation();

	UWxCombatLibrary::ApplyDamage(SourceASC, TargetASC, DamageInfo, HitResult, 0.f);
}
