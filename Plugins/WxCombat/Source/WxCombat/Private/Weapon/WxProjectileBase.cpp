// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxProjectileBase.h"
#include "AbilitySystem/Effect/WxEffect_HitStop.h"
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
#include "Damage/WxDamageTableRow.h"
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

FGenericTeamId AWxProjectileBase::GetGenericTeamId() const
{
	const IGenericTeamAgentInterface* InstigatorTeamAgent = Cast<IGenericTeamAgentInterface>(GetInstigator());
	return InstigatorTeamAgent ? InstigatorTeamAgent->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

void AWxProjectileBase::Reflect(APawn* Parrier)
{
	APawn* Shooter = GetInstigator();
	if (!Parrier || !Shooter)
	{
		return;
	}

	// 팀은 Instigator에서, 대미지 출처는 Owner에서 파생하므로 둘을 함께 옮겨야 되돌아간 히트가 패리한 쪽의 것이 된다.
	SetOwner(Parrier);
	SetInstigator(Parrier);

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

	// 되돌림이 출처를 갈아 끼우므로 대미지보다 먼저 읽는다.
	// 흘려낸 히트는 대미지 GE가 걸리지 않고, 가드를 뚫는 공격에는 퍼펙트 가드가 서지 않는다.
	const FWxDamageTableRow* DamageRow = DamageDataRow.GetRow<FWxDamageTableRow>(TEXT("HandleHitCollisionOverlap"));
	const bool bReflecting = bCanReflect && !bEvaded && DamageRow && DamageRow->bCanGuard
		&& TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_PerfectGuard);

	// 회피여도 호출은 그대로다 — 회피 성공 판정이 여기서 나가고, 대미지와 상태이상은 그쪽이 알아서 거른다.
	if (UWxCombatLibrary::ApplyDamage(this, OtherActor, DamageDataRow, HitResult))
	{
		UWxEffect_HitStop::Apply(InstigatorHitStop, SourceASC, SourceASC);
		UWxEffect_HitStop::Apply(VictimHitStop, SourceASC, TargetASC);
	}

	if (bReflecting)
	{
		Reflect(Cast<APawn>(OtherActor));
	}
	else if (!bEvaded)
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
