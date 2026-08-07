// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Effect/WxEffect_ResetDP.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MotionWarpingComponent.h"
#include "WxCombatLibrary.h"
#include "WxDamageInfo.h"
#include "WxGameplayTags.h"

namespace
{
	// 워프 타겟 이름. 두 변형(앞잡·뒤잡)의 공격 몽타주 MotionWarping 노티파이 Warp Target Name·Warp Point 를 이 값으로 맞춘다.
	const FName WarpTargetName = TEXT("Finisher");
}

UWxAbility_Finisher::UWxAbility_Finisher()
{
	// 발동은 상호작용(서버 권위)이 보내는 GameplayEvent 다. 대미지도 서버에서 적용하므로 ServerInitiated.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 처형은 상호작용으로 발동되는 실행형 반응이다.
	// 앞잡(그로기)은 플레이어의 공격으로 만들어지므로, 그로기 직후 F를 누르면 아직 활성인 공격의 BlockAbilitiesWithTag(Ability)에 걸려 발동이 거부된다(막힌 발동은 재시도 없이 소모).
	// HitReact·Groggy처럼 매칭될 애셋태그를 비워 두어 공격/스킬의 하드 차단에 막히지 않게 한다. (Ability.Finisher 태그는 어디서도 조회하지 않음)
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);

	// 피니시 연출 중에는 다른 어빌리티로 캔슬되지 않게 차단한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	
	// 피니시 중에는 무적
	ActivationOwnedTags.AddTag(WxGameplayTags::State_Invincible);

	// 처형 연출 진행 상태를 발행한다. 상호작용(WxAbility_Interact)이 이 태그에 막혀, 연출 도중 재입력으로 다른 대상을 상호작용해 몽타주가 겹치는 것을 차단한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::State_Finisher);

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
	// 대상에 가하는 변경은 전부 대상 ASC 를 거치고 액터 자체는 위치만 읽으므로 const 로 다룬다.
	const AActor* Target = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
	const FGameplayTag TriggerTag = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();

	// 트리거로 변형을 분기한다. 공격 몽타주·짝 피격 태그·종료 시 DP 리셋 여부만 변형별로 다르고 나머지 흐름은 공유한다.
	// 대미지는 두 변형이 같다 — 수치도 타이밍도 어빌리티가 아니라 공격 몽타주의 WxAnimNotify_FinisherDamage 가 결정한다.
	const bool bBackstab = (TriggerTag == WxGameplayTags::Event_Backstab);
	UAnimMontage* AttackerMontage = bBackstab ? BackstabMontage : FinisherMontage;
	const FGameplayTag VictimHitReactTag = bBackstab ? WxGameplayTags::Event_HitReact_Backstab : WxGameplayTags::Event_HitReact_Finisher;

	if (!AttackerMontage || !AvatarActor || !Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = Target;

	// 연출 진행 상태를 대상에게도 발행한다(권위 발행 + 복제).
	// 대상 액터가 이 태그로 자기 처형 어포던스를 닫으므로, 연출 중 다른 플레이어에게 프롬프트가 재노출되거나 중복 발동되지 않는다.
	if (ActorInfo->IsNetAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			TargetASC->AddLooseGameplayTag(WxGameplayTags::State_Finisher, 1, EGameplayTagReplicationState::TagOnly);
		}
	}

	// 1. 피해자 위치를 공유 앵커로, 공격자가 피해자를 바라보도록 워프 타겟 등록. 멈출 간격은 몽타주의 Warp Point가 소유한다.
	RegisterWarpTarget(AvatarActor, Target);

	// 2. 적에게 짝 피격 이벤트 송출 → HitReact 가 짝 피격 몽타주를 공격 몽타주와 동시 재생.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		FGameplayEventData VictimEvent;
		VictimEvent.Instigator = AvatarActor;
		VictimEvent.Target = Target;
		VictimEvent.EventTag = VictimHitReactTag;
		TargetASC->HandleGameplayEvent(VictimHitReactTag, &VictimEvent);
	}

	// 3. 공격자 몽타주 재생(몽타주의 WxAnimNotify_FinisherDamage 가 ApplyFinisherDamage 를 호출해 대미지 적용).
	// 피해자 짝 피격(WxAbility_HitReact)이 고정 1.0 으로 재생되므로, 처형 연출의 프레임 싱크를 위해 공격자도 ASPD 비의존 고정 1.0 으로 재생한다.
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackerMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 앞잡만 종료 시 대상 DP를 리셋해 그로기를 해제한다. 뒤잡은 애초에 그로기를 전제하지 않아 리셋할 DP가 없다.
	if (bBackstab)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Finisher::HandleMontageFinished);
	}
	else
	{
		MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Finisher::HandleFinisherMontageCompleted);
	}
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

	// 대상에 걸어둔 연출 진행 상태를 해제한다. 중단·캔슬도 이 경로를 지나므로 태그가 새지 않는다.
	// 대상이 대미지를 견뎌 살아남으면 여기서 어포던스가 정상 복구된다.
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor.Get()))
		{
			TargetASC->RemoveLooseGameplayTag(WxGameplayTags::State_Finisher, 1, EGameplayTagReplicationState::TagOnly);
		}
	}
	TargetActor = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Finisher::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Finisher::HandleFinisherMontageCompleted()
{
	// 앞잡 처형 종료 → 확정 대상의 DP를 0으로 리셋해 그로기를 해제한다.
	// 피해자 짝 피격 몽타주 완료에 의존하지 않고 공격자가 권위적으로 해제한다.
	if (const AActor* Target = TargetActor.Get())
	{
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		if (SourceASC && TargetASC)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			const FGameplayEffectSpecHandle ResetSpec = SourceASC->MakeOutgoingSpec(UWxEffect_ResetDP::StaticClass(), GetAbilityLevel(), Context);
			if (ResetSpec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*ResetSpec.Data.Get(), TargetASC);
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Finisher::RegisterWarpTarget(AActor* AvatarActor, const AActor* Target) const
{
	UMotionWarpingComponent* MotionWarping = AvatarActor ? AvatarActor->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!MotionWarping || !Target)
	{
		return;
	}

	// 앵커 = 피해자 위치. 공격자는 피해자를 바라보도록 회전한다.
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

	// 앞잡·뒤잡 모두 노티파이가 넘긴 같은 계수 피해를 적용한다 — 대미지는 변형에 따라 갈리지 않는다.
	UWxCombatLibrary::ApplyDamage(SourceASC, TargetASC, DamageInfo, HitResult, 0.f);
}
