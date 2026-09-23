// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxCombatLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "WxGameplayTags.h"

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

int32 AWxProjectileBase::GetProjectileLevel() const
{
	return ProjectileLevel;
}

FGenericTeamId AWxProjectileBase::GetGenericTeamId() const
{
	const IGenericTeamAgentInterface* InstigatorTeamAgent = Cast<IGenericTeamAgentInterface>(GetInstigator());
	return InstigatorTeamAgent ? InstigatorTeamAgent->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

void AWxProjectileBase::Reflect(APawn& Parrier)
{
	const APawn* Shooter = GetInstigator();
	if (!bCanReflect || !Shooter)
	{
		return;
	}

	// 팀은 Instigator에서, 대미지 출처는 Owner에서 파생하므로 둘을 함께 옮겨야 되돌아간 히트가 패리한 쪽의 것이 된다.
	SetOwner(&Parrier);
	SetInstigator(&Parrier);

	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Shooter->GetActorLocation());
	SetActorRotation(LookAtRotation);
	ProjectileMovement->Velocity = LookAtRotation.Vector() * ProjectileMovement->InitialSpeed;
	if (ProjectileMovement->bIsHomingProjectile)
	{
		ProjectileMovement->HomingTargetComponent = Shooter->GetRootComponent();
	}

	// 날아온 시간만큼 깎인 수명으로는 돌아가는 도중에 사라질 수 있다.
	SetLifeSpan(InitialLifeSpan);
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
			// 대상이 컴포넌트 단위라 부위 위치를 직접 조준한다.
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

void AWxProjectileBase::OnRep_Instigator()
{
	Super::OnRep_Instigator();

	// 유도 대상은 되돌림 전의 Instigator라 클라가 알 수 없어 비운다 — 궤적은 복제된 위치가 끌고 간다.
	ProjectileMovement->Velocity = GetActorRotation().Vector() * ProjectileMovement->InitialSpeed;
	ProjectileMovement->HomingTargetComponent = nullptr;
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

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

	// 무적 대상은 피해 없이 통과한다. 클라 충돌 연출과 서버 수명이 같은 조건을 쓴다.
	const bool bEvaded = TargetASC
		&& !TargetASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)
		&& TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_Invincible);

	if (!HasAuthority())
	{
		// 클라이언트는 충돌 연출만 낸다. 서버의 피해·수명·반사는 확정 결과를 따른다.
		if (!bEvaded)
		{
			PlayImpactFX();
		}
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

	// 회피여도 호출은 그대로다 — 무적의 Immunity가 피해를 막으며 회피 성공을 통지한다. 히트스톱과 퍼펙트 가드 되돌림은 피해 GE가 처리한다.
	const AActor* OwnerBeforeHit = GetOwner();
	UWxCombatLibrary::ApplyDamage(this, OtherActor, DamageDataRow, HitResult);
	if (bEvaded)
	{
		return;
	}

	PlayImpactFX();
	// 되돌려졌으면 막은 쪽으로 Owner가 바뀌어 있다.
	if (GetOwner() == OwnerBeforeHit)
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
