// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxWeaponBase::AWxWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
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
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	HitCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(GripPoint);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollision->SetCollisionObjectType(WxCollision::Attack);
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

void AWxWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromCharacter();

	Super::EndPlay(EndPlayReason);
}

void AWxWeaponBase::AttachToCharacter(ACharacter* Character, FName SocketName)
{
	if (!Character)
	{
		return;
	}

	AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	SetOwner(Character);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWxWeaponBase::DetachFromCharacter()
{
	// 활성 공격 구간이 남아있으면 강제 종료
	if (ActiveAttackCount > 0)
	{
		ActiveAttackCount = 0;
		HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HitActorsThisSwing.Empty();
		DamageInfo = FWxDamageInfo();

		if (AActor* OwnerActor = GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
			{
				ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
			}
		}
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
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
		HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
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
		HitActorsThisSwing.Empty();
		DamageInfo = FWxDamageInfo();

		if (AActor* OwnerActor = GetOwner())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
			{
				ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_WeaponCollision);
			}
		}
	}
}

void AWxWeaponBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 클라이언트와 서버 모두 동일한 히트 판정과 GE 적용을 수행한다.
	// 클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며,
	// 서버의 권위 적용과 불일치하면 GAS가 자동으로 롤백한다.

	AActor* WeaponOwner = GetOwner();
	if (!OtherActor || OtherActor == WeaponOwner)
	{
		return;
	}
	if (HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

	const IGenericTeamAgentInterface* OwnerTeam = Cast<IGenericTeamAgentInterface>(WeaponOwner);
	if (OwnerTeam)
	{
		const ETeamAttitude::Type Attitude = OwnerTeam->GetTeamAttitudeTowards(*OtherActor);
		if (Attitude != ETeamAttitude::Hostile)
		{
			return;
		}
	}

	HitActorsThisSwing.Add(OtherActor);

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner);
	if (!TargetASC || !SourceASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddInstigator(WeaponOwner, WeaponOwner);
	Context.SetAbility(SourceASC->GetAnimatingAbility());

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
	Context.AddHitResult(HitResult);

	const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(SourceASC, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		if (Spec.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}

	if (HitStopDuration > 0.f)
	{
		FGameplayCueParameters HitStopParams;
		HitStopParams.RawMagnitude = HitStopDuration;
		SourceASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_HitStop, HitStopParams);
	}
}
