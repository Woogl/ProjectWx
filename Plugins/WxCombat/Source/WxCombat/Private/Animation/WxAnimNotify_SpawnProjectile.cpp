// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_SpawnProjectile.h"
#include "Weapon/WxProjectileBase.h"
#include "WxGameplayTags.h"

void UWxAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !ProjectileClass)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const FVector SpawnLocation = MeshComp->GetSocketLocation(SpawnSocketName);
	const FRotator SpawnRotation = Owner->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWxProjectileBase* Projectile = Owner->GetWorld()->SpawnActorDeferred<AWxProjectileBase>(ProjectileClass, SpawnTransform, Owner, Cast<APawn>(Owner), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		if (bUnblockable)
		{
			Projectile->AttackTags.AddTag(WxGameplayTags::Damage_Unblockable);
		}
		Projectile->FinishSpawning(SpawnTransform);
	}
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	return TEXT("Spawn Projectile");
}
