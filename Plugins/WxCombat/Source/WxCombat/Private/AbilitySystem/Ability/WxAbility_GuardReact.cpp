// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_GuardReact.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "WxGameplayTags.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UWxAbility_GuardReact::UWxAbility_GuardReact()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_GuardReact);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_GuardReact);

	// 가드가 배타 점유 중인 채로 그 위에 얹혀야 한다.
	ActivationGroup = EWxAbilityActivationGroup::Override;

	// HitReact가 차단에 쓰는 것과 같은 태그라 한 히트에 둘 중 하나만 뜬다.
	// 이 검사는 서버에서만 돈다 — 소유 클라는 ClientActivateAbilitySucceedWithEventData가 CallActivateAbility를 직접 불러 태그 요건을 건너뛴다.
	ActivationRequiredTags.AddTag(WxGameplayTags::Ability_Guard);

	// 가드 중 연속 피격은 앞 연출을 끊고 새로 튼다.
	bRetriggerInstancedAbility = true;

	// 일반 피격은 부모 Event.Hit으로 오고, 반응 종류는 TargetTags의 HitReact 페이로드로 받는다.
	// 자식까지 등록하면 조상마다 한 번씩 발화해 같은 히트에 두 번 뜬다.
	FAbilityTriggerData HitTrigger;
	HitTrigger.TriggerTag = WxGameplayTags::Event_Hit;
	HitTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(HitTrigger);

	FAbilityTriggerData PerfectGuardTrigger;
	PerfectGuardTrigger.TriggerTag = WxGameplayTags::Event_PerfectGuard;
	PerfectGuardTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(PerfectGuardTrigger);
}

float UWxAbility_GuardReact::GetMontagePlayRate() const
{
	return 1.f;
}

#if WITH_EDITOR
EDataValidationResult UWxAbility_GuardReact::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	Result = CombineDataValidationResults(Result, ValidateInstantBlendIn(GuardHitReactMontage, Context));
	Result = CombineDataValidationResults(Result, ValidateInstantBlendIn(GuardKnockbackMontage, Context));
	Result = CombineDataValidationResults(Result, ValidateInstantBlendIn(GuardBreakMontage, Context));
	Result = CombineDataValidationResults(Result, ValidateInstantBlendIn(PerfectGuardMontage, Context));

	return Result;
}
#endif

void UWxAbility_GuardReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const FGameplayTag TriggerTag = TriggerEventData ? TriggerEventData->EventTag : WxGameplayTags::Event_Hit;
	const FGameplayTag ReactionTag = TriggerEventData
		? TriggerEventData->TargetTags.Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First()
		: FGameplayTag();

	if (TriggerTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		// 커밋·몽타주보다 먼저 끊어야 한다 — 어느 쪽이든 실패해 가드가 남으면 SP 회복이 Effect.GuardReduction에 막혀 다시는 깨지지 않는 상태가 된다.
		// 이 어빌리티는 서버와 소유 클라 양쪽에서 활성화되므로 취소도 양쪽에서 로컬로 일어난다.
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
			ASC->CancelAbilities(&GuardAbilityTags);
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !PlayMontage(SelectMontage(TriggerTag, ReactionTag)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 흡수 자세는 공격이 온 쪽을 봐야 성립하므로, 몽타주가 실제로 걸린 뒤에 방향을 맞춘다.
	// 공격자에게 부착된 원인 액터(근접 무기)는 위치가 스윙의 산물이라, 떨어져 날아온 것(투사체)만 자기 위치를 방향으로 쓴다.
	// 투사체는 히트 직후 파괴되므로 트리거 RPC가 늦게 닿는 소유 클라에서는 널로 풀려, 그때도 공격자로 떨어진다.
	const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	const AActor* Causer = TriggerEventData ? TriggerEventData->ContextHandle.GetEffectCauser() : nullptr;
	const AActor* AttackSource = (Causer && Causer->GetAttachParentActor() != Attacker) ? Causer : Attacker;

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (AttackSource && AvatarActor)
	{
		FVector Direction = AttackSource->GetActorLocation() - AvatarActor->GetActorLocation();
		Direction.Z = 0.0;
		if (!Direction.IsNearlyZero())
		{
			FRotator NewRotation = AvatarActor->GetActorRotation();
			NewRotation.Yaw = Direction.ToOrientationRotator().Yaw;
			AvatarActor->SetActorRotation(NewRotation);
		}
	}

	// 몽타주가 실제로 걸린 뒤에 건다 — 실패 경로에서 걸면 태스크가 같은 프레임에 파괴되며 복제된 딜레이션만 한 번 튄다.
	if (TriggerTag == WxGameplayTags::Event_PerfectGuard)
	{
		UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectGuardSlowTimeDilation, PerfectGuardSlowTimeDuration);
		SlowTimeTask->ReadyForActivation();
	}
}

void UWxAbility_GuardReact::HandleMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UAnimMontage* UWxAbility_GuardReact::SelectMontage(FGameplayTag TriggerTag, FGameplayTag ReactionTag) const
{
	if (TriggerTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		return GuardBreakMontage;
	}

	if (TriggerTag == WxGameplayTags::Event_PerfectGuard)
	{
		return PerfectGuardMontage;
	}

	const bool bIsKnockHit = ReactionTag == WxGameplayTags::HitReact_KnockBack
		|| ReactionTag == WxGameplayTags::HitReact_KnockDown
		|| ReactionTag == WxGameplayTags::HitReact_KnockUp;

	return (bIsKnockHit && GuardKnockbackMontage) ? GuardKnockbackMontage.Get() : GuardHitReactMontage.Get();
}
