// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnMinion.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_SpawnMinion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = WxGameplayTags::Event_SpawnMinion;
	Payload.Instigator = Owner;
	Payload.OptionalObject = this;
	Payload.OptionalObject2 = MeshComp;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}

FString UWxAnimNotify_SpawnMinion::GetNotifyName_Implementation() const
{
	if (const UClass* TargetClass = MinionClass.Get())
	{
		return TargetClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}

TSubclassOf<APawn> UWxAnimNotify_SpawnMinion::GetMinionClass() const
{
	return MinionClass;
}

FVector UWxAnimNotify_SpawnMinion::GetSpawnOffset() const
{
	return SpawnOffset;
}
