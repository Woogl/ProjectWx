// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxHitStopComponent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

UWxHitStopComponent::UWxHitStopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxHitStopComponent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent = Cast<UWxAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UWxHitStopComponent::HandleActiveGameplayEffectAdded);
		AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UWxHitStopComponent::HandleGameplayEffectRemoved);
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Effect_HitStop).AddUObject(this, &UWxHitStopComponent::HandleHitStopTagChanged);
	}
}

void UWxHitStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
		AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Effect_HitStop).RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UWxHitStopComponent::HandleActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (!Spec.Def || !Spec.Def->GetGrantedTags().HasTag(WxGameplayTags::Effect_HitStop))
	{
		return;
	}

	// 공격자 쪽은 스펙의 어빌리티가 아직 몽타주를 쥐고 있을 때만 센다. 복제본은 비복제 어빌리티 인스턴스가 비어 여기서 걸러지고, 남는 예측 인스턴스가 본인 화면의 지속시간을 정한다.
	const FGameplayEffectContextHandle& Context = Spec.GetContext();
	if (Context.GetInstigatorAbilitySystemComponent() == AbilitySystemComponent)
	{
		if (!AbilitySystemComponent->GetAnimatingAbility() || AbilitySystemComponent->GetAnimatingAbility() != Context.GetAbilityInstance_NotReplicated())
		{
			return;
		}
	}

	FrozenHandles.Add(Handle);
	RefreshAnimRateScale();
}

void UWxHitStopComponent::HandleGameplayEffectRemoved(const FActiveGameplayEffect& RemovedEffect)
{
	if (FrozenHandles.Remove(RemovedEffect.Handle) == 0)
	{
		return;
	}

	RefreshAnimRateScale();
}

void UWxHitStopComponent::HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshAnimRateScale();
}

void UWxHitStopComponent::RefreshAnimRateScale()
{
	USkeletalMeshComponent* Mesh = AbilitySystemComponent->AbilityActorInfo.IsValid() ? AbilitySystemComponent->AbilityActorInfo->SkeletalMeshComponent.Get() : nullptr;
	if (!Mesh)
	{
		return;
	}

	// 권위 태그가 걷혔으면 어느 머신에서든 푼다. 로컬이 아직 세고 있다면 그건 새어 나간 상태다.
	const bool bTagged = AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Effect_HitStop);

	// 로컬 인스턴스를 받는 머신에서는 그쪽이 이긴다 — 태그는 예측 인스턴스와 복제본이 겹쳐 RTT만큼 늦게 걷히므로, 그것으로 얼리면 본인 화면만 더 오래 멈춘다.
	// 시뮬 프록시에는 인스턴스가 오지 않으므로 태그가 유일한 신호다.
	const bool bHasLocalInstances = AbilitySystemComponent->IsOwnerActorAuthoritative()
		|| (AbilitySystemComponent->AbilityActorInfo.IsValid() && AbilitySystemComponent->AbilityActorInfo->IsLocallyControlled());

	const bool bFrozen = bTagged && (!bHasLocalInstances || !FrozenHandles.IsEmpty());

	Mesh->GlobalAnimRateScale = bFrozen ? 0.f : 1.f;
}
