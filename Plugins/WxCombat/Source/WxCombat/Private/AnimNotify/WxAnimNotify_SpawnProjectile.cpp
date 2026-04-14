// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnProjectile.h"
#include "WxDamageTableRow.h"
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

	const FVector SpawnLocation = MeshComp->GetSocketLocation(SpawnSocketName);
	const FRotator SpawnRotation = Owner->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	// DamageSpec을 BeginPlay 이전에 준비해야 하므로 Deferred 스폰 후 InitializeDamageSpec 호출
	AWxProjectileBase* Projectile = Owner->GetWorld()->SpawnActorDeferred<AWxProjectileBase>(
		ProjectileClass,
		SpawnTransform,
		Owner,
		Cast<APawn>(Owner),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Projectile)
	{
		return;
	}

	Projectile->InitializeDamageSpec(ResolveDamageInfo());
	Projectile->FinishSpawning(SpawnTransform);
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	return TEXT("Spawn Projectile");
}

FWxDamageInfo UWxAnimNotify_SpawnProjectile::ResolveDamageInfo() const
{
	if (const FWxDamageTableRow* Row = DamageDataRow.GetRow<FWxDamageTableRow>(TEXT("WxAnimNotify_SpawnProjectile")))
	{
		FWxDamageInfo Resolved;
		Resolved.ApplyTableRow(*Row);
		return Resolved;
	}

	return DamageInfo;
}
