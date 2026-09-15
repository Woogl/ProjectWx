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

	USkeletalMeshComponent* Mesh = AbilitySystemComponent->AbilityActorInfo->SkeletalMeshComponent.Get();
	if (!Mesh)
	{
		return;
	}

	// 원격 클라 폰의 서버 사본은 클라 무브 안에서만 포즈를 틱해 클라와 함께 멈춘다. 서버 태그로 세우면 클라가 아직 멈추지 않은 무브의 루트모션이 어긋난다.
	const bool bFreezeAnimation = IsFrozen() && !Mesh->bOnlyAllowAutonomousTickPose;
	Mesh->GlobalAnimRateScale = bFreezeAnimation ? 0.f : 1.f;
}
