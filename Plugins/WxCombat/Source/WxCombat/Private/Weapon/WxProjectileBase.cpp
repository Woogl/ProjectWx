// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Targeting/WxLockOnComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WxCombatLibrary.h"

AWxProjectileBase::AWxProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	HitCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));
	SetRootComponent(HitCollision);
	HitCollision->SetCollisionProfileName(TEXT("WxProjectile"));
	HitCollision->OnComponentBeginOverlap.AddDynamic(this, &AWxProjectileBase::HandleHitCollisionOverlap);
	HitCollision->OnComponentHit.AddDynamic(this, &AWxProjectileBase::HandleHitCollisionHit);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(HitCollision);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(HitCollision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->InitialSpeed = 500.f;
	ProjectileMovement->MaxSpeed = 500;

	TrailFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailFX"));
	TrailFX->SetupAttachment(HitCollision);
	TrailFX->bAutoActivate = true;

	InitialLifeSpan = 10.f;
}

void AWxProjectileBase::InitializeDamageSpec(const FWxDamageInfo& InDamageInfo)
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
	if (!SourceASC)
	{
		return;
	}

	CachedEffectContext = SourceASC->MakeEffectContext();
	CachedEffectContext.AddSourceObject(this);
	CachedEffectContext.AddInstigator(GetOwner(), GetInstigator());
	CachedEffectContext.SetAbility(SourceASC->GetAnimatingAbility());

	CachedSpecHandles = InDamageInfo.MakeSpecs(SourceASC, CachedEffectContext);
}

void AWxProjectileBase::Destroyed()
{
	if (ImpactFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactFX, GetActorLocation(), GetActorRotation());
	}

	Super::Destroyed();
}

void AWxProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(GetInstigator()))
	{
		if (AActor* LockOnTarget = LockOnComp->GetLockOnTarget())
		{
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), LockOnTarget->GetActorLocation());
			SetActorRotation(LookAtRotation);
			ProjectileMovement->Velocity = LookAtRotation.Vector() * ProjectileMovement->InitialSpeed;
			if (ProjectileMovement->bIsHomingProjectile)
			{
				ProjectileMovement->HomingTargetComponent = LockOnTarget->GetRootComponent();
			}
		}
	}
}

void AWxProjectileBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	if (!UWxCombatLibrary::IsHostile(GetInstigator(), OtherActor))
	{
		return;
	}

	if (CachedEffectContext.IsValid())
	{
		FHitResult HitResult;
		if (bFromSweep)
		{
			HitResult = SweepResult;
		}
		else
		{
			FVector ClosestPoint;
			if (OtherComp->GetClosestPointOnCollision(HitCollision->GetComponentLocation(), ClosestPoint) >= 0.f)
			{
				HitResult.ImpactPoint = ClosestPoint;
				HitResult.Location = ClosestPoint;
			}
			else
			{
				HitResult.ImpactPoint = OtherComp->GetComponentLocation();
				HitResult.Location = OtherComp->GetComponentLocation();
			}
		}
		CachedEffectContext.AddHitResult(HitResult, true);
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		for (const FGameplayEffectSpecHandle& SpecHandle : CachedSpecHandles)
		{
			if (SpecHandle.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	Destroy();
}

void AWxProjectileBase::HandleHitCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	Destroy();
}
