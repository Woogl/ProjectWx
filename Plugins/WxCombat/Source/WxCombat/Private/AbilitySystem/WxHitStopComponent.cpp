// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxHitStopComponent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
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
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Effect_HitStop).AddUObject(this, &UWxHitStopComponent::HandleHitStopTagChanged);
		RefreshFrozenState();
	}
}

void UWxHitStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Effect_HitStop).RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UWxHitStopComponent::IsFrozen() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Effect_HitStop);
}

void UWxHitStopComponent::HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshFrozenState();
}

void UWxHitStopComponent::RefreshFrozenState()
{
	if (!AbilitySystemComponent->AbilityActorInfo.IsValid())
	{
		return;
	}

	const bool bFrozen = IsFrozen();

	if (USkeletalMeshComponent* Mesh = AbilitySystemComponent->AbilityActorInfo->SkeletalMeshComponent.Get())
	{
		Mesh->GlobalAnimRateScale = bFrozen ? 0.f : 1.f;
	}
}
