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
	// 점유 차단만으로는 부족하다: 공격·스킬의 콤보 재발동 분기는 활성 Spec만 보고 자체 판정하므로 배타 판정을 건너뛴다.
	// Ability.Skill은 부모 태그라 슬롯별 Ability.Skill.1~4까지 함께 잡는다.
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Attack);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Skill);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_HitReact);

	// Ability.Death 차단이 사망 몽타주를 지킨다 — 대미지 연출은 사망 처리 뒤에 발행된다.
	// 풀면 히트리액트가 사망 몽타주를 끊어 래그돌 폴백으로 떨어뜨린다.
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_Guard);
	ActivationBlockedTags.AddTag(WxGameplayTags::Effect_SuperArmor);

	bRetriggerInstancedAbility = true;

	// GameplayEvent 트리거는 부모 체인을 거슬러 조회되므로 부모 하나로 Event.HitReact.* 전부를 받는다.
	// 자식을 함께 등록하면 조상마다 한 번씩 발화해 같은 피격에 몽타주가 재시작한다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

float UWxAbility_HitReact::GetMontagePlayRate() const
{
	return 1.f;
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

	FGameplayTag EventTag = TriggerEventData ? TriggerEventData->EventTag : WxGameplayTags::Event_HitReact_Normal;
	const AActor* Instigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// 그로기 중엔 날아가지 않는다 — 긴 넉 몽타주가 그로기 몽타주를 밀어내는 동안에도 DP 드레인은 돌아 그로기 창이 잘려나간다.
	// 그로기를 유발한 히트도 여기 걸린다. DP 적용이 그로기를 먼저 띄우고 반응 이벤트가 그 뒤에 오기 때문이다.
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		FGameplayTagContainer KnockTags;
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockBack);
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockDown);
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockUp);
		if (EventTag.MatchesAny(KnockTags))
		{
			EventTag = WxGameplayTags::Event_HitReact_Normal;
		}
	}

	// 몽타주 선택과 달리 부수 효과는 여러 종류가 한 갈래를 공유하므로 따로 가른다.
	if (EventTag == WxGameplayTags::Event_HitReact_KnockUp)
	{
		ACharacter* Character = Cast<ACharacter>(AvatarActor);
		if (const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
		{
			const FVector LaunchVelocity(0.f, 0.f, Movement->JumpZVelocity);
			Character->LaunchCharacter(LaunchVelocity, false, true);
		}
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_KnockBack
		|| EventTag == WxGameplayTags::Event_HitReact_KnockDown
		|| EventTag == WxGameplayTags::Event_HitReact_Parry
		|| EventTag == WxGameplayTags::Event_HitReact_Finisher
		|| EventTag == WxGameplayTags::Event_HitReact_Backstab)
	{
		FaceInstigator(AvatarActor, Instigator);
	}

	if (!PlayMontage(SelectMontage(EventTag)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

UAnimMontage* UWxAbility_HitReact::SelectMontage(FGameplayTag EventTag) const
{
	UAnimMontage* Montage = nullptr;

	if (EventTag == WxGameplayTags::Event_HitReact_KnockBack)
	{
		Montage = KnockbackMontage;
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_KnockDown)
	{
		Montage = KnockdownMontage;
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_KnockUp)
	{
		Montage = KnockupMontage;
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_Parry)
	{
		Montage = ParryReactMontage;
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_Finisher)
	{
		Montage = FinisherHitReactMontage;
	}
	else if (EventTag == WxGameplayTags::Event_HitReact_Backstab)
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
