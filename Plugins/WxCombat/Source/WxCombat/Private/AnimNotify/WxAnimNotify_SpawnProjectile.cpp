// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/WxProjectileBase.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = WxGameplayTags::Event_SpawnProjectile;
	Payload.Instigator = Owner;
	Payload.OptionalObject = this;
	Payload.OptionalObject2 = MeshComp;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	if (const UClass* TargetClass = ProjectileClass.Get())
	{
		return TargetClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}

TSubclassOf<AWxProjectileBase> UWxAnimNotify_SpawnProjectile::GetProjectileClass() const
{
	return ProjectileClass;
}

FName UWxAnimNotify_SpawnProjectile::GetSpawnSocketName() const
{
	return SpawnSocketName;
}
