// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_CommandMinion.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Minion/WxMinionSubsystem.h"

void UWxAnimNotify_CommandMinion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWxMinionSubsystem* MinionSubsystem = Owner ? UWorld::GetSubsystem<UWxMinionSubsystem>(Owner->GetWorld()) : nullptr;
	if (!MinionSubsystem)
	{
		return;
	}

	MinionSubsystem->TryActivateAbilityOnMinions(*Owner, AbilityTag, FGameplayEventData());
}

FString UWxAnimNotify_CommandMinion::GetNotifyName_Implementation() const
{
	if (AbilityTag.IsValid())
	{
		return AbilityTag.ToString();
	}

	return Super::GetNotifyName_Implementation();
}
