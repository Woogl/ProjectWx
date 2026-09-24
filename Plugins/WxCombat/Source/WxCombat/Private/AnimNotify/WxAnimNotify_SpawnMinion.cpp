// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Minion/WxMinionSubsystem.h"
#include "System/WxCombatDeveloperSettings.h"

FLinearColor UWxAnimNotify_SpawnMinion::GetEditorColor()
{
	return GetDefault<UWxCombatDeveloperSettings>()->CombatAnimNotifyColor;
}

UWxAnimNotify_SpawnMinion::UWxAnimNotify_SpawnMinion()
{
	LocalSpawnOffset.SetLocation(FVector(200.0f, 0.0f, 0.0f));
}

void UWxAnimNotify_SpawnMinion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	APawn* Owner = MeshComp ? Cast<APawn>(MeshComp->GetOwner()) : nullptr;
	UWxMinionSubsystem* MinionSubsystem = Owner ? UWorld::GetSubsystem<UWxMinionSubsystem>(Owner->GetWorld()) : nullptr;
	if (!MinionSubsystem)
	{
		return;
	}

	const FTransform SpawnTransform = LocalSpawnOffset * Owner->GetActorTransform();
	MinionSubsystem->SpawnMinion(*Owner, MinionClass, SpawnTransform);
}

FString UWxAnimNotify_SpawnMinion::GetNotifyName_Implementation() const
{
	FString ClassName = MinionClass ? MinionClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Summon: %s"), *ClassName);
}
