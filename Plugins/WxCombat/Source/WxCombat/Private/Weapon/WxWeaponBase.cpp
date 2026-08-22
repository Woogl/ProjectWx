// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "WxCollisionChannels.h"
#include "WxCombatLibrary.h"

AWxWeaponBase::AWxWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	GripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GripPoint"));
	SetRootComponent(GripPoint);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GripPoint);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	HitCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(GripPoint);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollision->SetCollisionObjectType(ECC_WxAttack);
	HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitCollision->OnComponentBeginOverlap.AddDynamic(this, &AWxWeaponBase::HandleHitCollisionOverlap);
}

AWxWeaponBase* AWxWeaponBase::FindWeapon(const AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}

	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
	{
		if (AWxWeaponBase* Weapon = Cast<AWxWeaponBase>(Attached))
		{
			return Weapon;
		}
	}

	return nullptr;
}

void AWxWeaponBase::BeginAttack(const FDataTableRowHandle& InDamageInfo)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 공격 구간마다 비워, 콤보 전환으로 ANS가 겹쳐도 이전 스윙의 피격 기록이 새 스윙을 막지 않게 한다.
	HitActorsThisSwing.Empty();

	// SetCollisionEnabled는 이미 겹쳐 있는 액터에 Overlap을 즉시 발생시키므로, DamageInfo가 그보다 먼저 준비돼야 한다.
	DamageInfo = InDamageInfo;

	if (ActiveAttackCount == 0)
	{
		// 첫 프레임 Sweep이 0 거리가 되도록 현재 트랜스폼으로 초기화한다.
		// 직전 위치를 모르는 채 임의 값이 들어가면 무관한 액터까지 Sweep에 잡힌다.
		PrevCapsuleLocation = HitCollision->GetComponentLocation();

		HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SetActorTickEnabled(true);
	}

	++ActiveAttackCount;
}

void AWxWeaponBase::EndAttack()
{
	if (ActiveAttackCount <= 0)
	{
		return;
	}

	--ActiveAttackCount;

	if (ActiveAttackCount == 0)
	{
		HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetActorTickEnabled(false);
		HitActorsThisSwing.Empty();
	}
}

void AWxWeaponBase::CancelAttack()
{
	if (ActiveAttackCount <= 0)
	{
		return;
	}

	ActiveAttackCount = 0;
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(false);
	HitActorsThisSwing.Empty();
}

void AWxWeaponBase::SetVisualMesh(USkeletalMesh* MeshAsset)
{
	if (!Mesh || !MeshAsset)
	{
		return;
	}

	Mesh->SetSkeletalMeshAsset(MeshAsset);
}

void AWxWeaponBase::AttachToCharacter(ACharacter* OwnerCharacter, FName SocketName)
{
	if (!OwnerCharacter || !Mesh)
	{
		return;
	}

	USkeletalMeshComponent* TargetMesh = OwnerCharacter->GetMesh();
	if (!TargetMesh)
	{
		return;
	}

	AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	SetOwner(OwnerCharacter);
}

void AWxWeaponBase::DetachFromCharacter()
{
	CancelAttack();

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

USkeletalMeshComponent* AWxWeaponBase::GetMesh() const
{
	return Mesh;
}

void AWxWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ActiveAttackCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrLocation = HitCollision->GetComponentLocation();
	const FQuat CurrRotation = HitCollision->GetComponentQuat();

	// 직전 프레임 위치 → 현재 위치 사이를 캡슐 모양으로 Sweep해서, Overlap 이벤트가 한 틱에 캡슐을 지나친 액터를 놓치는 터널링을 보완한다.
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(HitCollision->GetScaledCapsuleRadius(), HitCollision->GetScaledCapsuleHalfHeight());

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WxWeaponSweep), false);
	Params.AddIgnoredActor(this);
	if (AActor* OwnerActor = GetOwner())
	{
		Params.AddIgnoredActor(OwnerActor);
	}
	for (const TObjectPtr<AActor>& AlreadyHit : HitActorsThisSwing)
	{
		if (AlreadyHit)
		{
			Params.AddIgnoredActor(AlreadyHit.Get());
		}
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, PrevCapsuleLocation, CurrLocation, CurrRotation, ObjectParams, Shape, Params);

	for (const FHitResult& Hit : Hits)
	{
		ProcessHit(Hit.GetActor(), Hit);
	}

	PrevCapsuleLocation = CurrLocation;
}

void AWxWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromCharacter();

	Super::EndPlay(EndPlayReason);
}

void AWxWeaponBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

	ProcessHit(OtherActor, HitResult);
}

void AWxWeaponBase::ProcessHit(AActor* OtherActor, const FHitResult& HitResult)
{
	// 클라와 서버가 같은 히트 판정과 GE 적용을 수행한다.

	AActor* WeaponOwner = GetOwner();
	if (!OtherActor || OtherActor == WeaponOwner || HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

	HitActorsThisSwing.Add(OtherActor);
	UAbilitySystemComponent* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(WeaponOwner);
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	UWxCombatLibrary::ApplyDamage(OwnerASC, TargetASC, DamageInfo, HitResult, HitStopDuration);
}
