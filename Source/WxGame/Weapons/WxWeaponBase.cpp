// Copyright Woogle. All Rights Reserved.

#include "Weapons/WxWeaponBase.h"
#include "AbilitySystemComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxCollisionChannels.h"
#include "Combat/WxCombatLibrary.h"

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
}

AWxWeaponBase* AWxWeaponBase::FindWeapon(const AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}

	// 무기는 캐릭터가 무기 슬롯(ChildActorComponent)에 만들어 붙인다.
	// 부착 액터 목록은 슬롯 밖 액터도 들어오는 자리라 기준이 되지 못한다.
	TArray<UChildActorComponent*> WeaponSlots;
	Owner->GetComponents<UChildActorComponent>(WeaponSlots);
	for (const UChildActorComponent* WeaponSlot : WeaponSlots)
	{
		if (AWxWeaponBase* Weapon = Cast<AWxWeaponBase>(WeaponSlot->GetChildActor()))
		{
			return Weapon;
		}
	}

	return nullptr;
}

void AWxWeaponBase::BeginAttack(const FDataTableRowHandle& InDamageInfo, const FGuid& AttackId)
{
	// 피해는 권위 머신에서만 적용되므로 판정도 거기서만 켠다.
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !AttackId.IsValid() || ActiveAttacks.Contains(AttackId))
	{
		return;
	}

	// 공격 구간마다 비워, 같은 몽타주에서 앞 구간이 닫히기 전에 다음 구간이 열려도 앞 구간의 피격 기록이 새 구간을 막지 않게 한다.
	// 피해 행은 하나뿐이라 겹친 동안은 나중에 열린 구간의 행으로 판정한다 — 서로 다른 행의 겹침은 지원하지 않는다.
	HitActorsThisSwing.Empty();

	// SetCollisionEnabled는 이미 겹쳐 있는 액터에 Overlap을 즉시 발생시키므로, DamageInfo가 그보다 먼저 준비돼야 한다.
	DamageInfo = InDamageInfo;
	const uint64 Generation = ++AttackGeneration;
	const bool bFirstAttack = ActiveAttacks.IsEmpty();
	ActiveAttacks.Add(AttackId);

	if (bFirstAttack)
	{
		// 첫 프레임 Sweep이 0 거리가 되도록 현재 위치로 초기화해 임의 위치 Sweep을 막는다.
		PrevShapeLocations.SetNum(HitShapes.Num());
		for (int32 Index = 0; Index < HitShapes.Num(); ++Index)
		{
			PrevShapeLocations[Index] = HitShapes[Index]->GetComponentLocation();
			HitShapes[Index]->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			// 충돌을 켜는 호출 안에서 패리·그로기로 공격이 끝나거나 다음 공격이 시작될 수 있다.
			if (ActiveAttacks.IsEmpty() || AttackGeneration != Generation)
			{
				return;
			}
		}

		SetActorTickEnabled(true);
	}
}

void AWxWeaponBase::EndAttack(const FGuid& AttackId)
{
	if (ActiveAttacks.Remove(AttackId) == 0)
	{
		return;
	}

	if (ActiveAttacks.IsEmpty())
	{
		++AttackGeneration;
		for (UShapeComponent* Shape : HitShapes)
		{
			Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		SetActorTickEnabled(false);
		HitActorsThisSwing.Empty();
	}
	
}

void AWxWeaponBase::CancelAttack()
{
	if (ActiveAttacks.IsEmpty())
	{
		return;
	}

	ActiveAttacks.Reset();
	++AttackGeneration;

	for (UShapeComponent* Shape : HitShapes)
	{
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorTickEnabled(false);
	HitActorsThisSwing.Empty();
}

USkeletalMeshComponent* AWxWeaponBase::GetMesh() const
{
	return Mesh;
}

void AWxWeaponBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetComponents<UShapeComponent>(HitShapes);
	for (UShapeComponent* Shape : HitShapes)
	{
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shape->SetCollisionObjectType(ECC_WxAttack);
		Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
		Shape->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		// 스트리밍 레벨이 다시 보이면 PostInitializeComponents가 다시 불린다.
		Shape->OnComponentBeginOverlap.AddUniqueDynamic(this, &AWxWeaponBase::HandleHitShapeOverlap);
	}
}

void AWxWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ActiveAttacks.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

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

	// Overlap 이벤트가 한 틱에 형상을 지나친 액터를 놓치는 터널링을 보완한다.
	const uint64 Generation = AttackGeneration;
	for (int32 Index = 0; Index < HitShapes.Num(); ++Index)
	{
		UShapeComponent* Shape = HitShapes[Index];
		const FVector CurrLocation = Shape->GetComponentLocation();

		// 형상 자신의 응답을 넘겨야 Overlap 경로와 판정이 일치한다.
		// 기본값은 전 채널 Block이라 지형에서 Sweep이 잘리고 한 틱 다중 타격도 끊긴다.
		const FCollisionResponseParams ResponseParams(Shape->GetCollisionResponseToChannels());

		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(Hits, PrevShapeLocations[Index], CurrLocation, Shape->GetComponentQuat(), ECC_WxAttack, Shape->GetCollisionShape(), Params, ResponseParams);

		for (const FHitResult& Hit : Hits)
		{
			ProcessHit(Hit.GetActor(), Hit);
			if (ActiveAttacks.IsEmpty() || AttackGeneration != Generation)
			{
				return;
			}
		}

		PrevShapeLocations[Index] = CurrLocation;
	}
}

void AWxWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelAttack();

	Super::EndPlay(EndPlayReason);
}

void AWxWeaponBase::HandleHitShapeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FHitResult HitResult;
	if (bFromSweep)
	{
		HitResult = SweepResult;
	}
	else if (OtherComp)
	{
		FVector ClosestPoint;
		if (OtherComp->GetClosestPointOnCollision(OverlappedComponent->GetComponentLocation(), ClosestPoint) >= 0.f)
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
	// 판정은 권위 머신에서만 켜지고(BeginAttack), 큐·히트스톱도 서버 판정을 따른다.

	AActor* WeaponOwner = GetOwner();
	if (ActiveAttacks.IsEmpty() || !IsValid(OtherActor) || OtherActor == WeaponOwner || HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

	if (!UWxCombatLibrary::IsHostile(WeaponOwner, OtherActor))
	{
		return;
	}

	HitActorsThisSwing.Add(OtherActor);
	// 히트스톱은 피해 GE가 이 무기의 설정으로 건다.
	const FDataTableRowHandle CurrentDamageInfo = DamageInfo;
	UWxCombatLibrary::ApplyDamage(this, OtherActor, CurrentDamageInfo, HitResult);
}
