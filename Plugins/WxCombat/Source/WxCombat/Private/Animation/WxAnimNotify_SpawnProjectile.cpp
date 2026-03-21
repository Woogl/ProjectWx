// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_SpawnProjectile.h"
#include "Weapon/WxProjectileBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

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

	AWxProjectileBase* Projectile = Owner->GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
	if (Projectile)
	{
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner)))
		{
			if (AActor* LockOnTarget = WxASC->GetLockOnTarget())
			{
				UProjectileMovementComponent* ProjMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
				if (ProjMovement)
				{
					ProjMovement->HomingTargetComponent = LockOnTarget->GetRootComponent();
				}
			}
		}
	}
}

FString UWxAnimNotify_SpawnProjectile::GetNotifyName_Implementation() const
{
	return TEXT("Spawn Projectile");
}
