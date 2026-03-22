// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WxCollisionChannels.h"

AWxProjectileBase::AWxProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	SetRootComponent(Arrow);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Arrow);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HitCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(Arrow);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitCollision->SetCollisionObjectType(WxCollision::Attack);
	HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	HitCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitCollision->OnComponentBeginOverlap.AddDynamic(this, &AWxProjectileBase::HandleHitCollisionOverlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->InitialSpeed = 500.f;
	ProjectileMovement->MaxSpeed = 500;

	InitialLifeSpan = 10.f;
}

void AWxProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator())))
	{
		if (AActor* LockOnTarget = WxASC->GetLockOnTarget())
		{
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), LockOnTarget->GetActorLocation());
			SetActorRotation(LookAtRotation);
			ProjectileMovement->Velocity = LookAtRotation.Vector() * ProjectileMovement->InitialSpeed;
			ProjectileMovement->HomingTargetComponent = LockOnTarget->GetRootComponent();
		}
	}

	if (DamageEffectClass)
	{
		if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator()))
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddSourceObject(this);
			Context.AddInstigator(GetOwner(), GetInstigator());
			DamageEffectSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
		}
	}
}

void AWxProjectileBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	if (DamageEffectSpecHandle.IsValid())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
		}
	}

	Destroy();
}
