// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "WxCollisionChannels.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"

AWxWeaponBase::AWxWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	GripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GripPoint"));
	SetRootComponent(GripPoint);

#if WITH_EDITORONLY_DATA
	GripArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("GripArrow"));
	if (GripArrow)
	{
		GripArrow->SetupAttachment(GripPoint);
		GripArrow->SetArrowColor(FLinearColor::Red);
	}
#endif

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GripPoint);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	HitCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(GripPoint);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollision->SetCollisionObjectType(WxCollision::WxAttack);
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

void AWxWeaponBase::BeginAttack(const FWxDamageInfo& InDamageInfo)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 새 공격 구간마다 히트 목록을 초기화한다.
	// 콤보 전환 시 겹치는 ANS 사이에서도 이전 스윙의 피격 기록이 새 스윙을 막지 않는다.
	HitActorsThisSwing.Empty();

	// DamageInfo를 콜리전 활성화보다 먼저 설정한다.
	// SetCollisionEnabled 시 이미 겹쳐있는 액터에 대해 Overlap이 즉시 발생할 수 있으므로,
	// 그 전에 설정이 준비되어 있어야 한다.
	DamageInfo = InDamageInfo;

	if (ActiveAttackCount == 0)
	{
		// 첫 프레임 Sweep이 0 거리가 되도록 현재 트랜스폼으로 초기화. 직전 위치를 모르는 상태에서
		// 임의 값이 들어가면 무관한 액터까지 Sweep으로 잡힐 수 있다.
		PrevCapsuleLocation = HitCollision->GetComponentLocation();
		PrevCapsuleRotation = HitCollision->GetComponentQuat();

		HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SetActorTickEnabled(true);

		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
		{
			ASC->AddLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
		}
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
		DamageInfo = FWxDamageInfo();

		if (AActor* OwnerActor = GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
			{
				ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
			}
		}
	}
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

	// BP에서 추가한 외형 SkeletalMeshComponent("VisualOverride" 태그)가 있으면 그 컴포넌트에 부착한다.
	for (USceneComponent* Child : TargetMesh->GetAttachChildren())
	{
		if (USkeletalMeshComponent* SkelChild = Cast<USkeletalMeshComponent>(Child))
		{
			if (SkelChild->ComponentHasTag(TEXT("VisualOverride")))
			{
				TargetMesh = SkelChild;
				break;
			}
		}
	}

	AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	SetOwner(OwnerCharacter);
}

void AWxWeaponBase::DetachFromCharacter()
{
	// 활성 공격 구간이 남아있으면 강제 종료
	if (ActiveAttackCount > 0)
	{
		ActiveAttackCount = 0;
		HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetActorTickEnabled(false);
		HitActorsThisSwing.Empty();
		DamageInfo = FWxDamageInfo();

		if (AActor* OwnerActor = GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
			{
				ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
			}
		}
	}

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

	// 직전 프레임 위치 → 현재 위치 사이를 캡슐 모양으로 Sweep해서, Overlap 이벤트가
	// 한 틱에 캡슐을 지나친 액터를 놓치는 터널링을 보완한다.
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
	PrevCapsuleRotation = CurrRotation;
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
	// 클라이언트와 서버 모두 동일한 히트 판정과 GE 적용을 수행한다.
	// 클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며,
	// 서버의 권위 적용과 불일치하면 GAS가 자동으로 롤백한다.

	AActor* WeaponOwner = GetOwner();
	if (!OtherActor || OtherActor == WeaponOwner || HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

	if (!UWxCombatLibrary::IsHostile(WeaponOwner, OtherActor))
	{
		return;
	}

	HitActorsThisSwing.Add(OtherActor);
	UAbilitySystemComponent* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(WeaponOwner);
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	UWxCombatLibrary::ApplyDamage(OwnerASC, TargetASC, DamageInfo, HitResult, HitStopDuration);
}
