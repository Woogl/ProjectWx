// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_Rush.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotifyState_Rush::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->MovementColor;
}
#endif

UWxAnimNotifyState_Rush::UWxAnimNotifyState_Rush()
{
	bIsNativeBranchingPoint = true;
}

void UWxAnimNotifyState_Rush::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
	Super::BranchingPointNotifyBegin(Payload);
	SendSignal(Payload, true);
}

void UWxAnimNotifyState_Rush::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload)
{
	Super::BranchingPointNotifyEnd(Payload);
	SendSignal(Payload, false);
}

void UWxAnimNotifyState_Rush::SendSignal(const FBranchingPointNotifyPayload& Payload, bool bBegin) const
{
	AActor* Owner = Payload.SkelMeshComponent ? Payload.SkelMeshComponent->GetOwner() : nullptr;
	if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner)))
	{
		ASC->OnMontageNotifyState.Broadcast(this, Payload, bBegin);
	}
}

FString UWxAnimNotifyState_Rush::GetNotifyName_Implementation() const
{
	switch (TargetSource)
	{
	case EWxRushTarget::LockOnTarget:
		return TEXT("Rush: LockOn");
	case EWxRushTarget::Master:
		return TEXT("Rush: Master");
	case EWxRushTarget::Minion:
		return TEXT("Rush: Minion");
	default:
		return TEXT("Rush: None");
	}
}
