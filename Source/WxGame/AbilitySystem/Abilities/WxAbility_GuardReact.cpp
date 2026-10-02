// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_GuardReact.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "AbilitySystem/WxAbilityTargetData_Direction.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "GameFramework/RootMotionSource.h"
#include "WxGameplayTags.h"

const FName UWxAbility_GuardReact::GuardHitSectionName(TEXT("GuardHit"));
const FName UWxAbility_GuardReact::GuardKnockbackSectionName(TEXT("GuardKnockback"));
const FName UWxAbility_GuardReact::GuardBreakSectionName(TEXT("GuardBreak"));
const FName UWxAbility_GuardReact::PerfectGuardSectionName(TEXT("PerfectGuard"));

UWxAbility_GuardReact::UWxAbility_GuardReact()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_GuardReact);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_GuardReact);

	// Ability.GuardReact는 Ability.Action 밖이라 가드 중에도 발동한다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);

	// HitReact가 차단에 쓰는 것과 같은 태그라 한 히트에 둘 중 하나만 뜬다.
	// 이 검사는 서버에서만 돈다 — 소유 클라는 ClientActivateAbilitySucceedWithEventData가 CallActivateAbility를 직접 불러 태그 요건을 건너뛴다.
	ActivationRequiredTags.AddTag(WxGameplayTags::Ability_Action_Guard);

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

bool UWxAbility_GuardReact::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	// 패리는 공격자의 경직이라 HitReact가 맡는다.
	if (Payload->EventTag == WxGameplayTags::Event_Hit_Parry)
	{
		return false;
	}

	// 반응 없는 일반 히트로 기존 가드 반응 몽타주를 재트리거하지 않는다.
	// 가드 브레이크·퍼펙트 가드는 막는 쪽의 결과라 공격의 반응 태그를 요구하지 않는다.
	if (Payload->EventTag == WxGameplayTags::Event_Hit && !Payload->TargetTags.HasTag(WxGameplayTags::HitReact))
	{
		return false;
	}

	return Super::ShouldAbilityRespondToEvent(ActorInfo, Payload);
}

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
			const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Action_Guard);
			ASC->CancelAbilities(&GuardAbilityTags);
		}
	}

	const FName SectionName = SelectSection(TriggerTag, ReactionTag);
	KnockbackDirection = FVector::ZeroVector;
	if (SectionName == GuardKnockbackSectionName && TriggerEventData)
	{
		const FGameplayAbilityTargetData* DirectionData = TriggerEventData->TargetData.Get(0);
		if (DirectionData && DirectionData->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct())
		{
			const FVector Direction = static_cast<const FWxAbilityTargetData_Direction*>(DirectionData)->Direction;
			if (!Direction.ContainsNaN())
			{
				KnockbackDirection = Direction.GetSafeNormal2D();
			}
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !PlayMontage(GetMontage(), SectionName))
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
	if (AvatarActor && !KnockbackDirection.IsNearlyZero())
	{
		AvatarActor->SetActorRotation((-KnockbackDirection).Rotation());
		return;
	}
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
}

bool UWxAbility_GuardReact::PlayMontageInternal(UAnimMontage* Montage, FName StartSection)
{
	bWaitForKnockbackCompletion = StartSection == GuardKnockbackSectionName;
	if (!Super::PlayMontageInternal(Montage, StartSection))
	{
		return false;
	}
	if (StartSection == GuardKnockbackSectionName)
	{
		const int32 SectionIndex = Montage->GetSectionIndex(StartSection);
		const float PlayRate = GetMontagePlayRate() * Montage->RateScale;
		const float Duration = SectionIndex != INDEX_NONE && PlayRate > 0.f ? Montage->GetSectionLength(SectionIndex) / PlayRate : 0.f;
		if (Duration > 0.f)
		{
			UAbilityTask_ApplyRootMotionConstantForce* KnockbackTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
				this, TEXT("WxGuardKnockback"), KnockbackDirection, KnockbackDistance / Duration,
				Duration, false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
			KnockbackTask->ReadyForActivation();
		}
	}
	return true;
}

void UWxAbility_GuardReact::HandleMontageBlendOut()
{
	if (!bWaitForKnockbackCompletion)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

FName UWxAbility_GuardReact::SelectSection(FGameplayTag TriggerTag, FGameplayTag ReactionTag)
{
	if (TriggerTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		return GuardBreakSectionName;
	}

	if (TriggerTag == WxGameplayTags::Event_PerfectGuard)
	{
		return PerfectGuardSectionName;
	}

	const bool bIsKnockHit = ReactionTag == WxGameplayTags::HitReact_KnockBack
		|| ReactionTag == WxGameplayTags::HitReact_KnockDown
		|| ReactionTag == WxGameplayTags::HitReact_KnockUp;

	return bIsKnockHit ? GuardKnockbackSectionName : GuardHitSectionName;
}
