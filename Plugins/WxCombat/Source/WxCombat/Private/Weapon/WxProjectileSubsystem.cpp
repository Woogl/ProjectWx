// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapon/WxProjectileBase.h"

AWxProjectileBase* UWxProjectileSubsystem::SpawnProjectile(AActor& Owner, TSubclassOf<AWxProjectileBase> ProjectileClass, const FTransform& SpawnTransform, int32 ProjectileLevel)
{
	if (!Owner.HasAuthority() || !ProjectileClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	// BeginPlay와 초기 오버랩 전에 발사 레벨을 확정한다.
	SpawnParams.bDeferConstruction = true;
	SpawnParams.Owner = &Owner;
	SpawnParams.Instigator = Cast<APawn>(&Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWxProjectileBase* Projectile = GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
	if (Projectile)
	{
		Projectile->ProjectileLevel = ProjectileLevel;
		Projectile->FinishSpawning(SpawnTransform);
	}
	return Projectile;
}

bool UWxProjectileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
