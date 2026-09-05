// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Weapon/WxProjectileBase.h"
#include "Weapon/WxProjectileSubsystem.h"

void UWxAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWxProjectileSubsystem* ProjectileSubsystem = Owner ? UWorld::GetSubsystem<UWxProjectileSubsystem>(Owner->GetWorld()) : nullptr;
	if (!ProjectileSubsystem)
	{
		return;
	}

	const FTransform SpawnTransform(Owner->GetActorRotation(), MeshComp->GetSocketLocation(SpawnSocketName));
	ProjectileSubsystem->SpawnProjectile(*Owner, ProjectileClass, SpawnTransform);
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	if (const UClass* TargetClass = ProjectileClass.Get())
	{
		return TargetClass->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
