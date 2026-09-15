// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxHitStopComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "WxCombatModule.h"
#include "WxGameplayTags.h"

namespace
{
	// 이동 경로에 0 델타를 직접 전달하지 않도록 작은 양수 배율을 쓴다.
	constexpr float HitStopTimeDilation = 0.001f;
}

UWxHitStopComponent::UWxHitStopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxHitStopComponent::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Effect_HitStop, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UWxHitStopComponent::HandleHitStopTagChanged);
		SetFrozen(HasHitStop());
	}
}

void UWxHitStopComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Effect_HitStop, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}

	SetFrozen(false);

	Super::EndPlay(EndPlayReason);
}

bool UWxHitStopComponent::HasHitStop() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Effect_HitStop);
}

void UWxHitStopComponent::HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	SetFrozen(NewCount > 0);
}

void UWxHitStopComponent::SetFrozen(bool bFrozen)
{
	AActor* Owner = GetOwner();
	if (!Owner || bHitStopApplied == bFrozen)
	{
		return;
	}

	const float PreviousTimeDilation = Owner->CustomTimeDilation;
	if (bFrozen)
	{
		SavedCustomTimeDilation = Owner->CustomTimeDilation;
		Owner->CustomTimeDilation = SavedCustomTimeDilation * HitStopTimeDilation;
	}
	else
	{
		Owner->CustomTimeDilation = SavedCustomTimeDilation;
	}

	bHitStopApplied = bFrozen;
	UE_LOG(LogWxCombat, VeryVerbose, TEXT("HitStop %s: Actor=%s Role=%s CustomTimeDilation=%g -> %g"),
		bFrozen ? TEXT("Apply") : TEXT("Restore"), *Owner->GetName(),
		*UEnum::GetValueAsString(Owner->GetLocalRole()), PreviousTimeDilation, Owner->CustomTimeDilation);
}
