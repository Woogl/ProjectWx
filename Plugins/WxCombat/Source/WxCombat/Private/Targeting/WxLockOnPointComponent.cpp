// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxLockOnPointComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "WxGameplayTags.h"

UWxLockOnPointComponent::UWxLockOnPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 움직이는 캐릭터/본에 부착되어 매 프레임 위치가 갱신되어야 하므로 Movable로 둔다.
	Mobility = EComponentMobility::Movable;

	LockOnRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);
}

bool UWxLockOnPointComponent::CanBeLockedOn() const
{
	FGameplayTagContainer OwnedTags;
	if (const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	return LockOnRequirements.RequirementsMet(OwnedTags);
}

void UWxLockOnPointComponent::SetLockedOn(bool bNewLockedOn)
{
	if (bLockedOn == bNewLockedOn)
	{
		return;
	}

	bLockedOn = bNewLockedOn;
	OnLockedOnChanged.Broadcast(bLockedOn);
}

bool UWxLockOnPointComponent::IsLockedOn() const
{
	return bLockedOn;
}

USceneComponent* UWxLockOnPointComponent::ResolveLockOnTarget(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	TArray<UWxLockOnPointComponent*> Points;
	Actor->GetComponents<UWxLockOnPointComponent>(Points);
	for (UWxLockOnPointComponent* Point : Points)
	{
		if (Point && Point->CanBeLockedOn())
		{
			return Point;
		}
	}

	return nullptr;
}

void UWxLockOnPointComponent::GatherLockOnPoints(const AActor* Actor, TArray<USceneComponent*>& OutPoints)
{
	OutPoints.Reset();
	if (!Actor)
	{
		return;
	}

	TArray<UWxLockOnPointComponent*> Points;
	Actor->GetComponents<UWxLockOnPointComponent>(Points);
	OutPoints.Reserve(Points.Num());
	for (UWxLockOnPointComponent* Point : Points)
	{
		if (Point && Point->CanBeLockedOn())
		{
			OutPoints.Add(Point);
		}
	}
}
