// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapon/WxProjectileBase.h"

AWxProjectileBase* UWxProjectileSubsystem::SpawnProjectile(AActor& Owner, TSubclassOf<AWxProjectileBase> ProjectileClass, const FTransform& SpawnTransform)
{
	if (!Owner.HasAuthority() || !ProjectileClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = &Owner;
	SpawnParams.Instigator = Cast<APawn>(&Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
}

bool UWxProjectileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
