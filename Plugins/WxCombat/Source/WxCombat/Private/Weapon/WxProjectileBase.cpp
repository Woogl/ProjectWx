// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "Kismet/KismetMathLibrary.h"

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

void AWxProjectileBase::InitializeDamageSpec()
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

	FWxDamageInfo DamageInfo = FWxDamageInfo::FromDataRow(DamageDataRow);
	CachedSpecHandles = DamageInfo.MakeSpecs(SourceASC, CachedEffectContext);
}

void AWxProjectileBase::PlayImpactFX()
{
	if (ImpactFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactFX, GetActorLocation(), GetActorRotation());
	}
}

void AWxProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeDamageSpec();

	if (UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(GetInstigator()))
	{
		if (USceneComponent* LockOnTarget = LockOnComp->GetLockOnTarget())
		{
			// 대상이 컴포넌트 단위라 부위 위치를 직접 조준하고, 호밍도 그 컴포넌트를 그대로 따라간다.
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), LockOnTarget->GetComponentLocation());
			SetActorRotation(LookAtRotation);
			ProjectileMovement->Velocity = LookAtRotation.Vector() * ProjectileMovement->InitialSpeed;
			if (ProjectileMovement->bIsHomingProjectile)
			{
				ProjectileMovement->HomingTargetComponent = LockOnTarget;
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

	PlayImpactFX();

	// 대미지 적용과 파괴는 서버 권위로만 처리하고, 클라에서는 복제된 파괴로 사라진다.
	if (!HasAuthority())
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
		else if (OtherComp)
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

	PlayImpactFX();

	// 파괴는 서버 권위로만 처리한다.
	if (!HasAuthority())
	{
		return;
	}

	Destroy();
}
