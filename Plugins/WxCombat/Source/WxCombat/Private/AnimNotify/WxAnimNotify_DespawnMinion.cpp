// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_DespawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Minion/WxMinionSubsystem.h"
#if WITH_EDITOR
#include "WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_DespawnMinion::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->MiscColor;
}
#endif

void UWxAnimNotify_DespawnMinion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const APawn* Owner = MeshComp ? Cast<APawn>(MeshComp->GetOwner()) : nullptr;
	UWxMinionSubsystem* MinionSubsystem = Owner ? UWorld::GetSubsystem<UWxMinionSubsystem>(Owner->GetWorld()) : nullptr;
	if (!MinionSubsystem)
	{
		return;
	}

	MinionSubsystem->DespawnMinions(*Owner, MinionClass);
}

FString UWxAnimNotify_DespawnMinion::GetNotifyName_Implementation() const
{
	FString ClassName = MinionClass ? MinionClass->GetName() : TEXT("None");
	ClassName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("Despawn: %s"), *ClassName);
}
