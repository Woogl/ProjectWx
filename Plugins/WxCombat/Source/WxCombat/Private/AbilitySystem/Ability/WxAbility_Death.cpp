// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Death.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

UWxAbility_Death::UWxAbility_Death()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Death);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Death);
	
	// 모든 어빌리티 발동 불가 및 캔슬
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	
	ActivationGroup = EWxAbilityActivationGroup::Reaction;
	bCancelsRunningActions = true;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Death;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

float UWxAbility_Death::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 커밋하지 않는다 — 사망은 코스트·쿨다운이 없는 강제 전이이고, 커밋 실패가 곧 사망 미성립(Ability.Death 미부여)이 된다.

	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Avatar && Avatar->GetMesh())
	{
		// CollisionEnabled를 내리면 ShouldCreatePhysicsState가 false가 되어 피직스 바디가 통째로 파괴되고, 래그돌 진입에서 다시 만드는 왕복이 생긴다.
		Avatar->GetMesh()->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Ignore);
	}
	
	AAIController* AIController = Avatar ? Cast<AAIController>(Avatar->GetController()) : nullptr;
	if (UBrainComponent* Brain = AIController ? AIController->GetBrainComponent() : nullptr)
	{
		Brain->StopLogic(TEXT("Death"));
	}

	PlayDeathMontageOrRagdoll();
}

void UWxAbility_Death::HandleMontageCompleted()
{
}

void UWxAbility_Death::HandleMontageInterrupted()
{
	EnableRagdoll();
}

void UWxAbility_Death::HandleMontageCancelled()
{
	EnableRagdoll();
}

void UWxAbility_Death::PlayDeathMontageOrRagdoll()
{
	// HitReact 등 활성 몽타주는 PlayMontageAndWait가 BlendOut으로 인계받는다.
	if (!PlayMontage(DeathMontage))
	{
		EnableRagdoll();
	}
}

void UWxAbility_Death::EnableRagdoll()
{
	// 오너 클라 인스턴스도 이 경로에 들어오므로, 루스 태그가 중복 추가되지 않게 서버에서만 발행한다.
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::Event_Ragdoll, 1, EGameplayTagReplicationState::TagOnly);
	}
}
