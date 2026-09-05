// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Minion/WxMinionSubsystem.h"

void UWxAnimNotify_SpawnMinion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWxMinionSubsystem* MinionSubsystem = Owner ? UWorld::GetSubsystem<UWxMinionSubsystem>(Owner->GetWorld()) : nullptr;
	if (!MinionSubsystem)
	{
		return;
	}

	const FTransform SpawnTransform(Owner->GetActorRotation(), Owner->GetActorTransform().TransformPosition(SpawnOffset));
	MinionSubsystem->SpawnMinion(*Owner, MinionClass, SpawnTransform, MaxMinionCount);
}

FString UWxAnimNotify_SpawnMinion::GetNotifyName_Implementation() const
{
	if (const UClass* TargetClass = MinionClass.Get())
	{
		return TargetClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
