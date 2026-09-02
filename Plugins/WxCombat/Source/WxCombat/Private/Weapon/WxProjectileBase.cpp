// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "AbilitySystem/Effect/WxEffect_HitStop.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxCombatLibrary.h"
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

FGenericTeamId AWxProjectileBase::GetGenericTeamId() const
{
	const IGenericTeamAgentInterface* InstigatorTeamAgent = Cast<IGenericTeamAgentInterface>(GetInstigator());
	return InstigatorTeamAgent ? InstigatorTeamAgent->GetGenericTeamId() : FGenericTeamId::NoTeam;
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

	const APawn* InstigatorPawn = GetInstigator();
	if (UWxLockOnComponent* LockOnComp = InstigatorPawn ? InstigatorPawn->FindComponentByClass<UWxLockOnComponent>() : nullptr)
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

	if (!UWxCombatLibrary::IsHostile(GetInstigator(), OtherActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

	// 판정은 각 머신이 로컬로 낸다 — 이펙트가 권위 검사 앞에서 재생되기 때문이다.
	const bool bEvaded = UWxCombatLibrary::CheckDamage(SourceASC, TargetASC) == EWxDamageCheck::Evaded;
	if (!bEvaded)
	{
		PlayImpactFX();
	}

	if (!HasAuthority())
	{
		return;
	}

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

	// 회피여도 호출은 그대로다 — 회피 성공 판정이 여기서 나가고, 대미지와 상태이상은 그쪽이 알아서 거른다.
	if (UWxCombatLibrary::ApplyDamage(this, OtherActor, DamageDataRow, HitResult))
	{
		UWxEffect_HitStop::Apply(HitStopDuration, SourceASC, TargetASC);
	}

	if (!bEvaded)
	{
		Destroy();
	}
}

void AWxProjectileBase::HandleHitCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	PlayImpactFX();

	if (!HasAuthority())
	{
		return;
	}

	Destroy();
}
