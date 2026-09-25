// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Ability/WxAbility_PlayMontageOnce.h"
#include "AnimNotify/WxAnimNotify_FinisherDamage.h"
#include "AnimNotify/WxAnimNotify_FinisherVictim.h"
#include "AbilitySystem/Effect/WxEffect_Invincible.h"
#include "AbilitySystem/Effect/WxEffect_ResetGP.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MotionWarpingComponent.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"

namespace
{
	// 공격 몽타주 MotionWarping 노티파이 Warp Target Name을 이 값으로 맞춘다.
	const FName FinisherWarpTargetName = TEXT("Finisher");
}

UWxAbility_Finisher::UWxAbility_Finisher()
{
	// 발동도 대미지 적용도 서버 권위이므로 ServerInitiated.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Finisher);
	SetAssetTags(AssetTags);

	// Ability.Finisher는 상호작용의 공통 차단 목록에 없어 도중에 발동할 수 있고, Override는 외부 취소를 막는다.
	ActivationGroup = EWxAbilityActivationGroup::Override;

	ActivationOwnedEffects.Add(UWxEffect_Invincible::StaticClass());

	// 상호작용이 이 태그에 막혀, 연출 도중 재입력으로 다른 대상과 몽타주가 겹치는 것을 차단한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Finisher);

	// 적 상호작용이 자격(그로기 또는 비전투 후방)을 서버에서 검증한 뒤 같은 이벤트로 발동시킨다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Finisher;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	InteractionPrompt = NSLOCTEXT("WxAbility_Finisher", "InteractionPrompt", "Finisher");
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

	if (!GetMontage() || !AvatarActor || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = Target;

	// 짝 몽타주 부여와 피해는 권위만 하므로 노티파이 대기도 여기서만 건다. 종료하면 태스크와 함께 대기도 끝난다.
	if (ActorInfo->IsNetAuthority())
	{
		UAbilityTask_WaitGameplayEvent* VictimEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WxGameplayTags::Event_PlayFinisherVictimMontage, nullptr, true);
		VictimEventTask->EventReceived.AddDynamic(this, &UWxAbility_Finisher::HandleVictimMontageEvent);
		VictimEventTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* DamageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WxGameplayTags::Event_ApplyFinisherDamage, nullptr, true);
		DamageEventTask->EventReceived.AddDynamic(this, &UWxAbility_Finisher::HandleFinisherDamageEvent);
		DamageEventTask->ReadyForActivation();
	}

	RegisterWarpTarget(AvatarActor, Target);

	if (!PlayMontage(GetMontage()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Finisher::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor.Get()))
		{
			if (UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get())
			{
				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				const FGameplayEffectSpecHandle ResetSpec = SourceASC->MakeOutgoingSpec(UWxEffect_ResetGP::StaticClass(), GetAbilityLevel(), Context);
				if (ResetSpec.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToTarget(*ResetSpec.Data.Get(), TargetASC);
				}
			}
		}
	}
	TargetActor.Reset();

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
	// 몬스터가 플레이어를 향해 회전하는 것은 피해자에게 부여되는 UWxAbility_PlayMontageOnce가 담당한다.
	const FVector TargetLocation = Target->GetActorLocation();
	FVector Direction = TargetLocation - AvatarActor->GetActorLocation();
	Direction.Z = 0.0;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator WarpRotation = Direction.Rotation();
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(FinisherWarpTargetName, TargetLocation, WarpRotation);
}

void UWxAbility_Finisher::HandleVictimMontageEvent(FGameplayEventData Payload)
{
	const UWxAnimNotify_FinisherVictim* VictimNotify = Cast<UWxAnimNotify_FinisherVictim>(Payload.OptionalObject.Get());
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor.Get());
	if (!VictimNotify || !TargetASC)
	{
		return;
	}

	// 짝 피격은 부여와 동시에 시작하고, 몽타주가 끝나면 스펙까지 스스로 걷힌다.
	FGameplayEventData VictimEvent;
	VictimEvent.Instigator = GetAvatarActorFromActorInfo();
	VictimEvent.Target = TargetActor.Get();
	VictimEvent.OptionalObject = VictimNotify->VictimMontage;

	FGameplayAbilitySpec VictimSpec(UWxAbility_PlayMontageOnce::StaticClass(), 1);
	TargetASC->GiveAbilityAndActivateOnce(VictimSpec, &VictimEvent);
}

void UWxAbility_Finisher::HandleFinisherDamageEvent(FGameplayEventData Payload)
{
	const UWxAnimNotify_FinisherDamage* DamageNotify = Cast<UWxAnimNotify_FinisherDamage>(Payload.OptionalObject.Get());
	const AActor* Target = TargetActor.Get();
	if (!DamageNotify || !Target)
	{
		return;
	}

	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.Location = Target->GetActorLocation();
	UWxCombatLibrary::ApplyDamage(GetAvatarActorFromActorInfo(), Target, DamageNotify->DamageDataRow, HitResult);
}
