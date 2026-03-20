// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_SpawnProjectile.h"
#include "Weapon/WxProjectileBase.h"

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

	const FTransform SpawnTransform = MeshComp->GetSocketTransform(SpawnSocketName);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Cast<APawn>(Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Owner->GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	return TEXT("Spawn Projectile");
}
