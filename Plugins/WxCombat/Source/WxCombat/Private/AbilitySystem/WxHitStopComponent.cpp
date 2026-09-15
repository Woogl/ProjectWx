// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxHitStopComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
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
	if (!Owner)
	{
		return;
	}

	// 서버 사본은 클라가 느려진 무브를 따라 멈추므로, 배율까지 걸면 서버가 복제 전 무브의 델타를 잘라 보정이 난다.
	const ACharacter* Character = Cast<ACharacter>(Owner);
	const bool bApply = bFrozen && !(Character && Character->GetMesh()->bOnlyAllowAutonomousTickPose);
	if (bHitStopApplied == bApply)
	{
		return;
	}

	const float PreviousTimeDilation = Owner->CustomTimeDilation;
	if (bApply)
	{
		SavedCustomTimeDilation = Owner->CustomTimeDilation;
		Owner->CustomTimeDilation = SavedCustomTimeDilation * HitStopTimeDilation;
	}
	else
	{
		Owner->CustomTimeDilation = SavedCustomTimeDilation;
	}

	bHitStopApplied = bApply;
	UE_LOG(LogWxCombat, VeryVerbose, TEXT("HitStop %s: Actor=%s Role=%s CustomTimeDilation=%g -> %g"),
		bApply ? TEXT("Apply") : TEXT("Restore"), *Owner->GetName(),
		*UEnum::GetValueAsString(Owner->GetLocalRole()), PreviousTimeDilation, Owner->CustomTimeDilation);
}
