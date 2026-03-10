// Copyright Epic Games, Inc. All Rights Reserved.

#include "Projectile/WxProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

AWxProjectileBase::AWxProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic,  ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn,         ECR_Overlap);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AWxProjectileBase::HandleSphereOverlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AWxProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed     = MaxSpeed;
	SetLifeSpan(LifeSpanSeconds);
}

void AWxProjectileBase::SetDamageEffectSpecHandle(const FGameplayEffectSpecHandle& InHandle)
{
	DamageEffectSpecHandle = InHandle;
}

void AWxProjectileBase::HandleSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		if (DamageEffectSpecHandle.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
		}
	}

	Destroy();
}
