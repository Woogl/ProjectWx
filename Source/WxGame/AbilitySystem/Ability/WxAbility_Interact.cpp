// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "Component/WxInteractionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "WxGameplayTags.h"

UWxAbility_Interact::UWxAbility_Interact()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Interact);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	ActivationInputTag = WxGameplayTags::Input_Interact;
}

void UWxAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APawn* Avatar = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPrimitiveComponent* AvatarRoot = Cast<UPrimitiveComponent>(Avatar->GetRootComponent());
	if (!AvatarRoot)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<UPrimitiveComponent*> OverlappingComponents;
	AvatarRoot->GetOverlappingComponents(OverlappingComponents);

	UWxInteractionComponent* Closest = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();
	const FVector AvatarLocation = Avatar->GetActorLocation();

	for (UPrimitiveComponent* OverlappingComponent : OverlappingComponents)
	{
		UWxInteractionComponent* InteractionComponent = Cast<UWxInteractionComponent>(OverlappingComponent);
		if (!InteractionComponent)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(AvatarLocation, InteractionComponent->GetComponentLocation());
		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			Closest = InteractionComponent;
		}
	}

	if (Closest)
	{
		Closest->TryInteract(Avatar);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
