// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Combat/WxLockOnComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

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

	if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator()))
	{
		CachedEffectContext = SourceASC->MakeEffectContext();
		CachedEffectContext.AddSourceObject(this);
		CachedEffectContext.AddInstigator(GetOwner(), GetInstigator());
		CachedEffectContext.SetAbility(SourceASC->GetAnimatingAbility());

		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectClasses)
		{
			if (!EffectClass)
			{
				continue;
			}

			FGameplayEffectSpecHandle EffectSpec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, CachedEffectContext);
			if (EffectSpec.IsValid())
			{
				EffectSpec.Data->AppendDynamicAssetTags(AttackTags);
				EffectSpec.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, ATKCoeff);
			}
			EffectSpecHandles.Add(EffectSpec);
		}
	}
}

void AWxProjectileBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}
	
	const IGenericTeamAgentInterface* OwnerTeam = Cast<IGenericTeamAgentInterface>(GetInstigator());
	if (OwnerTeam)
	{
		const ETeamAttitude::Type Attitude = OwnerTeam->GetTeamAttitudeTowards(*OtherActor);
		if (Attitude != ETeamAttitude::Hostile)
		{
			return;
		}
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
		for (const FGameplayEffectSpecHandle& SpecHandle : EffectSpecHandles)
		{
			if (SpecHandle.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}

	Destroy();
}
