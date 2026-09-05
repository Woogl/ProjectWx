// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Minion/WxMinionSubsystem.h"

UWxAnimNotify_SpawnMinion::UWxAnimNotify_SpawnMinion()
{
	LocalSpawnOffset.SetLocation(FVector(200.0f, 0.0f, 0.0f));
}

void UWxAnimNotify_SpawnMinion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWxMinionSubsystem* MinionSubsystem = Owner ? UWorld::GetSubsystem<UWxMinionSubsystem>(Owner->GetWorld()) : nullptr;
	if (!MinionSubsystem)
	{
		return;
	}

	MinionSubsystem->SpawnMinion(*Owner, MinionClass, LocalSpawnOffset, MaxMinionCount);
}

FString UWxAnimNotify_SpawnMinion::GetNotifyName_Implementation() const
{
	if (const UClass* TargetClass = MinionClass.Get())
	{
		return TargetClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
