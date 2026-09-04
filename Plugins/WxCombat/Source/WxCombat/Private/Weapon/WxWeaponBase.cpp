// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "AbilitySystem/Effect/WxEffect_HitStop.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
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
		// 첫 프레임 Sweep이 0 거리가 되도록 현재 위치로 초기화해 임의 위치 Sweep을 막는다.
		PrevShapeLocations.SetNum(HitShapes.Num());
		for (int32 Index = 0; Index < HitShapes.Num(); ++Index)
		{
			PrevShapeLocations[Index] = HitShapes[Index]->GetComponentLocation();
			HitShapes[Index]->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

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
	if (ActiveAttackCount <= 0)
	{
		return;
	}

	ActiveAttackCount = 0;

	for (UShapeComponent* Shape : HitShapes)
	{
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorTickEnabled(false);
	HitActorsThisSwing.Empty();
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
		Shape->OnComponentBeginOverlap.AddDynamic(this, &AWxWeaponBase::HandleHitShapeOverlap);
	}
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
	for (int32 Index = 0; Index < HitShapes.Num(); ++Index)
	{
		UShapeComponent* Shape = HitShapes[Index];
		const FVector CurrLocation = Shape->GetComponentLocation();

		// 형상 자신의 응답을 넘겨야 Overlap 경로와 판정이 일치한다. 기본값은 전 채널 Block이라 지형에서 Sweep이 잘리고 한 틱 다중 타격도 끊긴다.
		const FCollisionResponseParams ResponseParams(Shape->GetCollisionResponseToChannels());

		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(Hits, PrevShapeLocations[Index], CurrLocation, Shape->GetComponentQuat(), ECC_WxAttack, Shape->GetCollisionShape(), Params, ResponseParams);

		for (const FHitResult& Hit : Hits)
		{
			ProcessHit(Hit.GetActor(), Hit);
		}

		PrevShapeLocations[Index] = CurrLocation;
	}
}

void AWxWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromCharacter();

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
	// 클라와 서버가 같은 히트 판정과 GE 적용을 수행한다.

	AActor* WeaponOwner = GetOwner();
	if (!OtherActor || OtherActor == WeaponOwner || HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

	// 아군·중립에는 대미지도 대미지 행의 AdditionalEffects도 걸리면 안 되므로, 적용 앞에서 막는다.
	if (!UWxCombatLibrary::IsHostile(WeaponOwner, OtherActor))
	{
		return;
	}

	HitActorsThisSwing.Add(OtherActor);
	if (UWxCombatLibrary::ApplyDamage(this, OtherActor, DamageInfo, HitResult))
	{
		// 대미지 GE 뒤라야 공격자 쪽은 동기로 도착한 반응(패리 등)에 몽타주를 양보하고, 피격자 쪽은 막 시작된 반응 몽타주를 얼린다.
		UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner);
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		UWxEffect_HitStop::Apply(InstigatorHitStop, OwnerASC, OwnerASC);
		UWxEffect_HitStop::Apply(VictimHitStop, OwnerASC, TargetASC);
	}
}
