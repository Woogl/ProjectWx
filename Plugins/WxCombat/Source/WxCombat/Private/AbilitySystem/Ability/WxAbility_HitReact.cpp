// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_HitReact);
	SetAssetTags(AssetTags);

	ActivationGroup = EWxAbilityActivationGroup::Override;

	// 진행 중인 것은 공격·스킬만 끊는다 — 부류(그룹)로 끊으면 적 패턴까지 평타 피격에 중단된다.
	// Override는 막히지 않을 뿐 남을 끊지는 않는다 — 진행 중인 공격을 실제로 중단시키려면 지목이 필요하다.
	// Ability.Skill은 부모 태그라 슬롯별 Ability.Skill.1~4까지 함께 잡는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_HitReact);

	// 사망·무적·슈퍼아머는 거는 쪽(사망 어빌리티·각 GE)이 이 어빌리티를 막으므로 여기에 더하지 않는다.
	// 가드는 방어 판정(Effect.GuardReduction)이 아니라 어빌리티로 막는다 — GuardReact의 요구 태그와 같은 것을 봐야 한 히트에 둘 다 거부되는 상태가 없다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Guard);

	bRetriggerInstancedAbility = true;

	FAbilityTriggerData HitTrigger;
	HitTrigger.TriggerTag = WxGameplayTags::Event_Hit;
	HitTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(HitTrigger);
}

bool UWxAbility_HitReact::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	// 가드 브레이크는 GuardReact가 맡는다.
	if (Payload->EventTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		return false;
	}

	// ActivateAbility에서 거르면 재트리거 종료와 공격·스킬 취소가 이미 끝난 뒤다.
	const FGameplayTag ReactionTag = GetReactionTag(*Payload);
	if (!ReactionTag.IsValid() || !HitReactMontage || !HitReactMontage->IsValidSectionName(ReactionTag.GetTagLeafName()))
	{
		return false;
	}

	return Super::ShouldAbilityRespondToEvent(ActorInfo, Payload);
}

float UWxAbility_HitReact::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 회전·띄우기는 커밋과 몽타주가 모두 성립한 뒤에 낸다 — 어느 하나라도 실패해 곧장 종료하면 캐릭터가 어빌리티 없이 공중에 뜬다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 기본 반응은 없다 — 대미지 행이 HitReactTag 를 비운 것이 곧 "이 공격은 피격 반응을 일으키지 않는다"는 지정이다.
	const FGameplayTag ReactionTag = TriggerEventData ? GetReactionTag(*TriggerEventData) : FGameplayTag();
	if (!ReactionTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// 그로기 중 넉 계열은 보내는 쪽(UWxEffectComponent_DamageReaction)이 일반 피격으로 낮춰 온다.
	if (!PlayMontage(HitReactMontage, ReactionTag.GetTagLeafName()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ReactionTag == WxGameplayTags::HitReact_KnockUp)
	{
		if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
		{
			Character->LaunchCharacter(FVector(0.f, 0.f, KnockupZVelocity), false, true);
		}
	}
	else if (ReactionTag == WxGameplayTags::HitReact_KnockBack
		|| ReactionTag == WxGameplayTags::HitReact_KnockDown
		|| ReactionTag == WxGameplayTags::Event_Hit_Parry)
	{
		FaceInstigator(AvatarActor, Instigator);
	}
}

FGameplayTag UWxAbility_HitReact::GetReactionTag(const FGameplayEventData& Payload)
{
	// 패리는 반응 태그와 무관하게 받는다 — 성립 여부는 대미지 행의 bCanParry가 이미 갈랐다.
	if (Payload.EventTag == WxGameplayTags::Event_Hit_Parry)
	{
		return WxGameplayTags::Event_Hit_Parry;
	}

	return Payload.TargetTags.Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First();
}

void UWxAbility_HitReact::FaceInstigator(AActor* AvatarActor, const AActor* Instigator)
{
	if (!AvatarActor || !Instigator)
	{
		return;
	}

	FVector Direction = Instigator->GetActorLocation() - AvatarActor->GetActorLocation();
	Direction.Z = 0.0;
	if (!Direction.IsNearlyZero())
	{
		AvatarActor->SetActorRotation(Direction.ToOrientationRotator());
	}
}
