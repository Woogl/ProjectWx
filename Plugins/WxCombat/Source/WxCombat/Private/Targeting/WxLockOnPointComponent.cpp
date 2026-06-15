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

	// 기본값: 죽은 대상은 락온 불가. 다른 조건은 엔티티별로 BP 에서 오버라이드한다.
	LockOnRequirements.IgnoreTags.AddTag(WxGameplayTags::State_Dead);

#if WITH_EDITORONLY_DATA
	// 보이지 않는 마커이므로 에디터에서 위치를 스프라이트로 표시해 배치·선택을 돕는다.
	bVisualizeComponent = true;
#endif
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

USceneComponent* UWxLockOnPointComponent::ResolveLockOnTarget(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// 락온 가능한 첫 지점을 반환한다. 지점이 없거나 모두 불가하면 nullptr(락온 대상이 될 수 없다).
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

	// 락온 가능한 지점만 후보로 모은다.
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
