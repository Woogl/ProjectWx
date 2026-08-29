// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
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
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_SuperArmor);

	// 가드는 방어 판정(Effect.Guard)이 아니라 어빌리티로 막는다 — GuardReact의 요구 태그와 같은 것을 봐야 한 히트에 둘 다 거부되는 상태가 없다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Guard);

	bRetriggerInstancedAbility = true;

	FAbilityTriggerData HitTrigger;
	HitTrigger.TriggerTag = WxGameplayTags::Event_Hit;
	HitTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(HitTrigger);
}

float UWxAbility_HitReact::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 가드 브레이크는 GuardReact가 전담한다. 부모 Event.Hit 트리거로 여기에도 닿을 수 있으므로 먼저 제외한다.
	if (TriggerEventData && TriggerEventData->EventTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 회전·띄우기는 커밋과 몽타주가 모두 성립한 뒤에 낸다 — 어느 하나라도 실패해 곧장 종료하면 캐릭터가 어빌리티 없이 공중에 뜬다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	FGameplayTag ReactionTag = WxGameplayTags::HitReact_Normal;
	if (TriggerEventData)
	{
		ReactionTag = TriggerEventData->TargetTags.Filter(FGameplayTagContainer(WxGameplayTags::HitReact)).First();
		if (!ReactionTag.IsValid() && TriggerEventData->EventTag == WxGameplayTags::Event_Hit_Parry)
		{
			ReactionTag = WxGameplayTags::Event_Hit_Parry;
		}
	}

	// 반응 태그 없이 Event.Hit만 온 평타는 가드 리액션만 처리한다.
	if (!ReactionTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// 그로기 중엔 날아가지 않는다 — 긴 넉 몽타주가 그로기 몽타주를 밀어내는 동안에도 DP 드레인은 돌아 그로기 창이 잘려나간다.
	// 그로기를 유발한 히트도 여기 걸린다. DP 적용이 그로기를 먼저 띄우고 피격 이벤트가 그 뒤에 오기 때문이다.
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy)
		&& (ReactionTag == WxGameplayTags::HitReact_KnockBack
			|| ReactionTag == WxGameplayTags::HitReact_KnockDown
			|| ReactionTag == WxGameplayTags::HitReact_KnockUp))
	{
		ReactionTag = WxGameplayTags::HitReact_Normal;
	}

	if (!PlayMontage(SelectMontage(ReactionTag)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 선택과 달리 부수 효과는 여러 종류가 한 갈래를 공유하므로 따로 가른다.
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
	else if (ReactionTag == WxGameplayTags::Event_Hit_Parry)
	{
		Montage = ParryReactMontage;
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
