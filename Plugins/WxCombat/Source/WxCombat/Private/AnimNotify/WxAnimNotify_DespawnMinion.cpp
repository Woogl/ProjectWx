// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_DespawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Minion/WxMinionSubsystem.h"
#include "System/WxCombatDeveloperSettings.h"

FLinearColor UWxAnimNotify_DespawnMinion::GetEditorColor()
{
	return GetDefault<UWxCombatDeveloperSettings>()->CombatAnimNotifyColor;
}

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
	// 같은 소환물을 부르는 소환 노티파이가 클래스 이름만 띄우므로, 타임라인에서 구별되게 접두어를 붙인다.
	if (const UClass* TargetClass = MinionClass.Get())
	{
		return FString::Printf(TEXT("Despawn %s"), *TargetClass->GetName());
	}

	return Super::GetNotifyName_Implementation();
}
