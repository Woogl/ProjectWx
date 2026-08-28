// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "AbilitySystem/Ability/WxAbility_BeingFinished.h"
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

	// 처형은 상호작용 어빌리티가 대상에게 넘긴 이벤트로 동기 발동하므로, 그 상호작용이 아직 점유 중일 때 뜬다.
	// Reaction이라 그 점유에 막히지 않는다 — 상호작용은 곧바로 스스로 끝나므로 끊어 줄 필요는 없다.
	ActivationGroup = EWxAbilityActivationGroup::Reaction;

	ActivationOwnedEffects.Add(UWxEffect_Invincible::StaticClass());

	// 상호작용이 이 태그에 막혀, 연출 도중 재입력으로 다른 대상과 몽타주가 겹치는 것을 차단한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Finisher);

	// 앞잡·뒤잡이 같은 상호작용 이벤트를 받고, 페이로드에 실린 대상 소유 태그로 하나만 성립한다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Finisher;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	TargetRequiredTags.AddTag(WxGameplayTags::Ability_Groggy);
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

	if (!AttackerMontage || !AvatarActor || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = Target;

	// 어빌리티 부여는 권위에서만 — 클라에서 부르면 엔진이 거부하며 Error를 남긴다.
	if (ActorInfo->IsNetAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			// 짝 피격은 부여와 동시에 시작하고, 몽타주가 끝나면 스펙까지 스스로 걷힌다. 대미지는 노티파이 몫이라 이벤트엔 몽타주만 싣는다.
			FGameplayEventData VictimEvent;
			VictimEvent.Instigator = AvatarActor;
			VictimEvent.Target = Target;
			VictimEvent.OptionalObject = VictimMontage;

			FGameplayAbilitySpec VictimSpec(UWxAbility_BeingFinished::StaticClass(), 1);
			TargetASC->GiveAbilityAndActivateOnce(VictimSpec, &VictimEvent);
		}
	}

	RegisterWarpTarget(AvatarActor, Target);

	if (!PlayMontage(AttackerMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Finisher::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 중단·캔슬도 이 경로를 지나므로 그로기 해제가 몽타주 종료 방식과 무관하게 한 번 일어난다.
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
	// 몬스터가 플레이어를 향해 회전하는 것은 처형 당하기 어빌리티가 담당한다.
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

void UWxAbility_Finisher::ApplyFinisherDamage(const FDataTableRowHandle& DamageInfo) const
{
	const AActor* Target = TargetActor.Get();

	// 처형은 무기 액터를 거치지 않고 노티파이가 직접 넣으므로 공격자 자신이 원인 액터다.
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Target || !Avatar)
	{
		return;
	}

	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.Location = Target->GetActorLocation();

	UWxCombatLibrary::ApplyDamage(Avatar, Target, DamageInfo, HitResult, 0.f);
}
