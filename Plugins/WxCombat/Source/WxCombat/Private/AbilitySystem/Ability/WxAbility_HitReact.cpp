// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// 항상 서버에서 발행된 GameplayEvent로 트리거된다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_HitReact);
	SetAssetTags(AssetTags);

	ActivationGroup = EWxAbilityActivationGroup::Reaction;

	// 진행 중인 것은 공격·스킬만 끊는다 — 부류(그룹)로 끊으면 적 패턴까지 평타 피격에 중단된다.
	// Reaction은 막히지 않을 뿐 남을 끊지는 않는다 — 진행 중인 공격을 실제로 중단시키려면 지목이 필요하다.
	// Ability.Skill은 부모 태그라 슬롯별 Ability.Skill.1~4까지 함께 잡는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_HitReact);

	// 사망 몽타주는 사망 쪽 BlockAbilitiesWithTag가 지키므로 여기에 Ability.Death를 더하지 않는다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_Guard);
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_SuperArmor);

	bRetriggerInstancedAbility = true;

	// 반응 종류는 트리거 태그가 아니라 페이로드에 실려 오므로 트리거는 피격 이벤트 하나다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Hit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

float UWxAbility_HitReact::GetMontagePlayRate() const
{
	return 1.f;
}

bool UWxAbility_HitReact::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	return Super::ShouldAbilityRespondToEvent(ActorInfo, Payload) && Payload->TargetTags.HasTag(WxGameplayTags::HitReact);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 회전·띄우기보다 먼저 판정한다 — 커밋이 막힌 뒤에 연출만 남으면 캐릭터가 어빌리티 없이 공중에 뜬다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	FGameplayTag ReactionTag = TriggerEventData ? TriggerEventData->TargetTags.Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First() : WxGameplayTags::HitReact_Normal;
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// 그로기 중엔 날아가지 않는다 — 긴 넉 몽타주가 그로기 몽타주를 밀어내는 동안에도 DP 드레인은 돌아 그로기 창이 잘려나간다.
	// 그로기를 유발한 히트도 여기 걸린다. DP 적용이 그로기를 먼저 띄우고 피격 이벤트가 그 뒤에 오기 때문이다.
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		FGameplayTagContainer KnockTags;
		KnockTags.AddTagFast(WxGameplayTags::HitReact_KnockBack);
		KnockTags.AddTagFast(WxGameplayTags::HitReact_KnockDown);
		KnockTags.AddTagFast(WxGameplayTags::HitReact_KnockUp);
		if (ReactionTag.MatchesAny(KnockTags))
		{
			ReactionTag = WxGameplayTags::HitReact_Normal;
		}
	}

	// 몽타주 선택과 달리 부수 효과는 여러 종류가 한 갈래를 공유하므로 따로 가른다.
	if (ReactionTag == WxGameplayTags::HitReact_KnockUp)
	{
		ACharacter* Character = Cast<ACharacter>(AvatarActor);
		if (const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
		{
			const FVector LaunchVelocity(0.f, 0.f, Movement->JumpZVelocity);
			Character->LaunchCharacter(LaunchVelocity, false, true);
		}
	}
	else if (ReactionTag == WxGameplayTags::HitReact_KnockBack
		|| ReactionTag == WxGameplayTags::HitReact_KnockDown
		|| ReactionTag == WxGameplayTags::HitReact_Parry
		|| ReactionTag == WxGameplayTags::HitReact_Finisher
		|| ReactionTag == WxGameplayTags::HitReact_Backstab)
	{
		FaceInstigator(AvatarActor, Instigator);
	}

	if (!PlayMontage(SelectMontage(ReactionTag)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

UAnimMontage* UWxAbility_HitReact::SelectMontage(FGameplayTag ReactionTag) const
{
	UAnimMontage* Montage = nullptr;

	if (ReactionTag == WxGameplayTags::HitReact_KnockBack)
	{
		Montage = KnockbackMontage;
	}
	else if (ReactionTag == WxGameplayTags::HitReact_KnockDown)
	{
		Montage = KnockdownMontage;
	}
	else if (ReactionTag == WxGameplayTags::HitReact_KnockUp)
	{
		Montage = KnockupMontage;
	}
	else if (ReactionTag == WxGameplayTags::HitReact_Parry)
	{
		Montage = ParryReactMontage;
	}
	else if (ReactionTag == WxGameplayTags::HitReact_Finisher)
	{
		Montage = FinisherHitReactMontage;
	}
	else if (ReactionTag == WxGameplayTags::HitReact_Backstab)
	{
		Montage = BackstabHitReactMontage;
	}

	return Montage ? Montage : NormalHitReactMontage.Get();
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
